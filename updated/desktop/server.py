"""Local drawing interface for the updated recovered CutOut C++ implementation.

Added in 2026 by Codex for Aaron Wetzler. The native segmentation code retains
Elad Richardson's and its third-party authors' notices. No external service is
used. This interface starts one isolated CLI process per segmentation request.
"""

from __future__ import annotations

import argparse
import base64
import binascii
import json
import math
from pathlib import Path
import struct
import subprocess
import tempfile
import threading
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer

HERE = Path(__file__).resolve().parent
REPO = HERE.parent.parent
MAX_REQUEST = 24 * 1024 * 1024
GATE = threading.BoundedSemaphore(1)


def decode_png(encoded: str) -> tuple[bytes, int, int]:
    if not isinstance(encoded, str) or not encoded.startswith("data:image/png;base64,"):
        raise ValueError("Upload an image through the drawing interface.")
    try:
        image = base64.b64decode(encoded.split(",", 1)[1], validate=True)
    except (ValueError, binascii.Error) as exc:
        raise ValueError("The image encoding is invalid.") from exc
    if len(image) < 33 or image[:8] != b"\x89PNG\r\n\x1a\n" or image[12:16] != b"IHDR":
        raise ValueError("Expected a PNG image.")
    width, height = struct.unpack(">II", image[16:24])
    if not width or not height or width > 8192 or height > 8192 or width * height > 4_000_000:
        raise ValueError("Use an image with at most four million pixels.")
    return image, width, height


def validate_request(body: dict) -> tuple[bytes, dict]:
    if not isinstance(body, dict):
        raise ValueError("Expected an image and scribbles.")
    image, width, height = decode_png(body.get("image", ""))
    scribbles = body.get("scribbles")
    if not isinstance(scribbles, dict):
        raise ValueError("Draw foreground and background scribbles first.")
    total = 0
    for kind in ("foreground", "background"):
        strokes = scribbles.get(kind)
        if not isinstance(strokes, list) or not strokes or len(strokes) > 256:
            raise ValueError("Draw at least one foreground and one background stroke.")
        for stroke in strokes:
            if not isinstance(stroke, list) or not stroke:
                raise ValueError("A stroke cannot be empty.")
            total += len(stroke)
            if total > 100_000:
                raise ValueError("Too many scribble points; clear and redraw shorter strokes.")
            for point in stroke:
                if not isinstance(point, list) or len(point) != 2:
                    raise ValueError("Invalid scribble coordinates.")
                x, y = point
                if any(isinstance(v, bool) or not isinstance(v, (int, float)) or
                       (isinstance(v, float) and not math.isfinite(v)) for v in point):
                    raise ValueError("Invalid scribble coordinates.")
                if not 0 <= x < width or not 0 <= y < height:
                    raise ValueError("A scribble is outside the image.")
    radius = scribbles.get("brush_radius", 2)
    if isinstance(radius, bool) or not isinstance(radius, int) or not 1 <= radius <= 15:
        raise ValueError("Invalid brush radius.")
    return image, scribbles


class Handler(BaseHTTPRequestHandler):
    def send_data(self, code: int, data: bytes, content_type: str) -> None:
        self.send_response(code)
        self.send_header("Content-Type", content_type)
        self.send_header("Content-Length", str(len(data)))
        self.send_header("Cache-Control", "no-store")
        self.send_header("X-Content-Type-Options", "nosniff")
        self.end_headers()
        self.wfile.write(data)

    def send_json(self, code: int, data: dict) -> None:
        self.send_data(code, json.dumps(data, allow_nan=False).encode(), "application/json")

    def do_GET(self) -> None:
        routes = {"/": ("index.html", "text/html; charset=utf-8"),
                  "/app.js": ("app.js", "text/javascript; charset=utf-8"),
                  "/styles.css": ("styles.css", "text/css; charset=utf-8")}
        if self.path in routes:
            filename, mime = routes[self.path]
            self.send_data(200, (HERE / filename).read_bytes(), mime)
        elif self.path == "/api/demo":
            self.send_fixture("coffee.png", "image/png")
        elif self.path == "/api/demo-scribbles":
            self.send_fixture("coffee-scribbles.json", "application/json")
        else:
            self.send_json(404, {"error": "Not found."})

    def send_fixture(self, filename: str, content_type: str) -> None:
        path = REPO / "updated/validation/fixtures" / filename
        if not path.is_file():
            self.send_json(404, {"error": "The example files are missing from this checkout."})
            return
        self.send_data(200, path.read_bytes(), content_type)

    def do_POST(self) -> None:
        if self.path != "/api/segment":
            self.send_json(404, {"error": "Not found."})
            return
        origin = self.headers.get("Origin")
        expected = {f"http://127.0.0.1:{self.server.server_port}", f"http://localhost:{self.server.server_port}"}
        if origin and origin not in expected:
            self.send_json(403, {"error": "Open the local CutOut page to run segmentation."})
            return
        if self.headers.get_content_type() != "application/json":
            self.send_json(415, {"error": "Expected JSON."})
            return
        try:
            length = int(self.headers.get("Content-Length", "0"))
            if not 0 < length <= MAX_REQUEST:
                raise ValueError("The request is too large or empty.")
            body = json.loads(self.rfile.read(length))
            image, scribbles = validate_request(body)
            masks = None
            if "foreground_mask" in body or "background_mask" in body:
                foreground, fw, fh = decode_png(body.get("foreground_mask", ""))
                background, bw, bh = decode_png(body.get("background_mask", ""))
                _, width, height = decode_png(body["image"])
                if (fw, fh) != (width, height) or (bw, bh) != (width, height):
                    raise ValueError("Scribble masks must match the image dimensions.")
                masks = foreground, background
            density = body.get("density", "figtree")
            distance = body.get("distance", "pixel")
            alpha = body.get("alpha", "refined")
            working = body.get("max_dimension", 400)
            if density not in ("figtree", "histogram") or distance not in ("pixel", "levelset") or alpha not in ("hard", "refined"):
                raise ValueError("Choose a supported segmentation mode.")
            if isinstance(working, bool) or not isinstance(working, int) or working not in (0, 400, 800):
                raise ValueError("Choose a supported working size.")
        except (ValueError, TypeError, RecursionError) as exc:
            self.send_json(400, {"error": str(exc)})
            return
        if not GATE.acquire(blocking=False):
            self.send_json(429, {"error": "A segmentation is running. Try again when it finishes."})
            return
        try:
            with tempfile.TemporaryDirectory(prefix="cutout-drawing-") as directory:
                work = Path(directory)
                (work / "input.png").write_bytes(image)
                (work / "scribbles.json").write_text(json.dumps(scribbles, allow_nan=False))
                command = [str(self.server.binary), "--image", str(work / "input.png"),
                           "--output-dir", str(work / "result"),
                           "--density", density, "--distance", distance, "--alpha", alpha,
                           "--max-dimension", str(working)]
                if masks:
                    (work / "foreground.png").write_bytes(masks[0])
                    (work / "background.png").write_bytes(masks[1])
                    command += ["--foreground", str(work / "foreground.png"), "--background", str(work / "background.png")]
                else:
                    command += ["--scribbles", str(work / "scribbles.json")]
                result = subprocess.run(command, text=True, capture_output=True, timeout=90)
                if result.returncode:
                    self.send_json(422, {"error": (result.stderr or result.stdout or "The native process failed.")[-1800:]})
                    return
                outputs = {}
                for name in ("mask", "cutout", "outline", "foreground_likelihood"):
                    outputs[name] = "data:image/png;base64," + base64.b64encode((work / "result" / (name + ".png")).read_bytes()).decode()
                metadata = json.loads((work / "result/result.json").read_text())
                self.send_json(200, {"images": outputs, "result": metadata})
        except subprocess.TimeoutExpired:
            self.send_json(504, {"error": "Segmentation exceeded 90 seconds. Try a smaller image or working size."})
        except (OSError, ValueError) as exc:
            self.send_json(500, {"error": "Could not read the native result: " + str(exc)[:300]})
        finally:
            GATE.release()


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--binary", type=Path, default=REPO / "build/native/cutout_cli")
    parser.add_argument("--port", type=int, default=8768)
    args = parser.parse_args()
    binary = args.binary.expanduser().resolve()
    if not binary.is_file():
        parser.error(f"Native CLI not found: {binary}. Build updated/native first.")
    server = ThreadingHTTPServer(("127.0.0.1", args.port), Handler)
    server.binary = binary
    print(f"CutOut updated runtime: http://127.0.0.1:{server.server_port}/", flush=True)
    print("Images stay on this computer. Ctrl-C stops the interface.", flush=True)
    try:
        server.serve_forever()
    except KeyboardInterrupt:
        pass
    finally:
        server.server_close()


if __name__ == "__main__":
    main()
