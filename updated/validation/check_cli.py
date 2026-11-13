#!/usr/bin/env python3
"""Exercise CLI I/O and error handling, without claiming photographic accuracy.

Requires Pillow for PNG generation and inspection. All generated inputs and outputs
stay beneath --output-dir. The included CC0 photograph is read without modification.
"""
from __future__ import annotations
import argparse
from datetime import datetime, timezone
import hashlib
import json
from pathlib import Path
import platform
import subprocess
import sys
from PIL import Image, ImageDraw

HERE = Path(__file__).resolve().parent

def require(condition, message):
    if not condition:
        raise AssertionError(message)

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--binary", type=Path, required=True)
    parser.add_argument("--output-dir", type=Path, required=True)
    parser.add_argument("--timeout", type=int, default=90)
    args = parser.parse_args()
    args.binary = args.binary.resolve()
    output = args.output_dir.resolve()
    output.mkdir(parents=True, exist_ok=True)
    sources = output / "inputs"
    sources.mkdir(exist_ok=True)
    photo = HERE / "fixtures/coffee.png"
    photo_seeds = HERE / "fixtures/coffee-scribbles.json"
    expected_hash = json.loads((HERE / "fixtures/PROVENANCE.json").read_text())["files"][0]["sha256"]
    require(hashlib.sha256(photo.read_bytes()).hexdigest() == expected_hash, "photo fixture hash changed")
    source_image = Image.new("RGB", (96, 64), (40, 120, 30))
    ImageDraw.Draw(source_image).rectangle((24, 13, 71, 50), fill=(220, 40, 30))
    source_image.save(sources / "two-tone.png")
    Image.new("RGB", (32, 24), (100, 100, 100)).save(sources / "constant.png")
    source_image.resize((768, 512), Image.Resampling.NEAREST).save(sources / "large.png")
    strokes = {"foreground": [[[36, 24], [52, 36]]], "background": [[[5, 5], [5, 55]]], "brush_radius": 1}
    (sources / "two-tone.json").write_text(json.dumps(strokes))
    constant_strokes = {"foreground": [[[20, 12]]], "background": [[[3, 3]]], "brush_radius": 0}
    (sources / "constant.json").write_text(json.dumps(constant_strokes))
    large_strokes = {key: [[[x*8,y*8] for x,y in stroke] for stroke in strokes[key]]
                     for key in ("foreground", "background")}
    large_strokes["brush_radius"] = 2
    (sources / "large.json").write_text(json.dumps(large_strokes))
    rows = []
    def run(name, image, scribbles, density="figtree", distance="pixel", alpha="hard", extra=(), expect_error=False):
        target = output / name
        # A rerun must inspect files from this invocation, never stale success
        # outputs left by an earlier binary. Other files are left untouched.
        for filename in ("mask.png", "cutout.png", "outline.png", "alpha.png",
                         "foreground_likelihood.png", "result.json"):
            (target / filename).unlink(missing_ok=True)
        command = [str(args.binary), "--image", str(image), "--scribbles", str(scribbles),
                   "--output-dir", str(target), "--density", density, "--distance", distance,
                   "--alpha", alpha, *extra]
        process = subprocess.run(command, capture_output=True, text=True, timeout=args.timeout)
        record = {"name": name, "density": density, "distance": distance, "alpha": alpha,
                  "exit_code": process.returncode, "expected_error": expect_error}
        if expect_error:
            require(process.returncode != 0, name + ": invalid input accepted")
            require(not (target / "result.json").exists(), name + ": failed run wrote success metadata")
            record["diagnostic"] = process.stderr.strip()[-1200:]
        else:
            require(process.returncode == 0, name + ": " + process.stderr[-2000:])
            metadata = json.loads((target / "result.json").read_text())
            dimensions = Image.open(image).size
            require((metadata["width"], metadata["height"]) == dimensions, name + ": metadata dimensions differ")
            require(metadata["density"] == density and metadata["distance"] == distance and metadata["alpha"] == alpha,
                    name + ": requested operator not recorded")
            mask = Image.open(target / "mask.png"); mask.load()
            rgba = Image.open(target / "cutout.png"); rgba.load()
            alpha_image = Image.open(target / "alpha.png"); alpha_image.load()
            outline = Image.open(target / "outline.png"); outline.load()
            likelihood = Image.open(target / "foreground_likelihood.png"); likelihood.load()
            require(mask.size == dimensions and mask.mode == "L", name + ": mask format/dimensions")
            require(rgba.size == dimensions and rgba.mode == "RGBA", name + ": transparent PNG format/dimensions")
            require(outline.size == dimensions and outline.mode == "RGB", name + ": outline format/dimensions")
            require(alpha_image.size == dimensions and alpha_image.mode == "L", name + ": alpha format/dimensions")
            require(likelihood.size == (metadata["working_width"], metadata["working_height"]), name + ": likelihood dimensions")
            require(set(mask.tobytes()) <= {0, 255}, name + ": nonbinary mask")
            require(rgba.getchannel("A").tobytes() == alpha_image.tobytes(), name + ": cutout alpha mismatch")
            if alpha == "hard":
                require(alpha_image.tobytes() == mask.tobytes(), name + ": hard alpha mismatch")
            seeds = json.loads(Path(scribbles).read_text())
            for label, expected in (("foreground", 255), ("background", 0)):
                for stroke in seeds[label]:
                    for point in stroke:
                        require(mask.getpixel(tuple(point)) == expected, name + ": explicit seed not respected")
                        require(alpha_image.getpixel(tuple(point)) == expected, name + ": alpha seed not respected")
            count = mask.tobytes().count(255)
            require(metadata["foreground_pixels"] == count, name + ": mask count disagrees with metadata")
            require(0 < count < dimensions[0]*dimensions[1], name + ": mask has no foreground/background")
            record["result"] = metadata
            record["file_sha256"] = {name: hashlib.sha256((target/name).read_bytes()).hexdigest()
                                     for name in ("mask.png","cutout.png","alpha.png","outline.png","foreground_likelihood.png")}
        record["passed"] = True
        rows.append(record)
        print("PASS", name, flush=True)
        return target
    # Every recovered density/propagation switch runs on a real photograph;
    # hard and original boundary-band alpha are recorded separately.
    first_photo = None
    photo_targets = {}
    for density in ("figtree", "histogram"):
        for distance in ("pixel", "levelset"):
            for alpha in ("hard", "refined"):
                target=run(f"coffee-{density}-{distance}-{alpha}",photo,photo_seeds,density,distance,alpha)
                photo_targets[density,distance,alpha]=target
                if first_photo is None: first_photo=target
    for density in ("figtree", "histogram"):
        for distance in ("pixel", "levelset"):
            hard=photo_targets[density,distance,"hard"] / "mask.png"
            refined=photo_targets[density,distance,"refined"] / "mask.png"
            require(hard.read_bytes()==refined.read_bytes(), "alpha refinement changed the photographic binary mask")
    repeat=run("coffee-figtree-pixel-hard-repeat",photo,photo_seeds)
    require((repeat/"mask.png").read_bytes()==(first_photo/"mask.png").read_bytes(),"repeat photographic mask differs")
    for density in ("figtree", "histogram"):
        for distance in ("pixel", "levelset"):
            run(f"constant-{density}-{distance}",sources/"constant.png",sources/"constant.json",density,distance)
    run("reduced-large",sources/"large.png",sources/"large.json",extra=("--max-dimension","96"))
    run("unreduced",sources/"two-tone.png",sources/"two-tone.json",extra=("--max-dimension","0","--no-smooth"))
    # Errors must produce a clear failure rather than accessing memory or
    # continuing with an incomplete/ambiguous set of user labels.
    invalid_inputs = {
        "empty-foreground": {"foreground":[],"background":[[[3,3]]]},
        "overlapping-labels": {"foreground":[[[3,3]]],"background":[[[3,3]]]},
        "coordinate-outside": {"foreground":[[[96,0]]],"background":[[[3,3]]]},
        "fractional-coordinate": {"foreground":[[[20.5,20]]],"background":[[[3,3]]]},
        "negative-brush": {"foreground":[[[20,20]]],"background":[[[3,3]]],"brush_radius":-1},
    }
    for name,value in invalid_inputs.items():
        path=sources/(name+".json");path.write_text(json.dumps(value))
        run("reject-"+name,sources/"two-tone.png",path,expect_error=True)
    path=sources/"malformed.json";path.write_text('{"foreground":')
    run("reject-malformed-json",sources/"two-tone.png",path,expect_error=True)
    run("reject-unknown-density",sources/"two-tone.png",sources/"two-tone.json",density="unknown",expect_error=True)
    run("reject-huge-histogram",sources/"two-tone.png",sources/"two-tone.json",extra=("--histogram-bins","999999"),expect_error=True)
    run("reject-seed-collision",sources/"two-tone.png",sources/"two-tone.json",extra=("--max-dimension","2"),expect_error=True)
    run("reject-missing-image",sources/"missing.png",sources/"two-tone.json",expect_error=True)
    report = {"created_utc":datetime.now(timezone.utc).isoformat(),
              "environment":{"platform":platform.platform(),"machine":platform.machine(),"python":sys.version.split()[0]},
              "scope":"CLI I/O, error handling, repeatability and retained-operator execution. One photograph without an independent reference mask; no photographic accuracy benchmark.",
              "all_four_hard_refined_masks_identical":True,
              "fixture_sha256":expected_hash,"cases":rows,"passed":all(row["passed"] for row in rows)}
    (output/"cli-validation.json").write_text(json.dumps(report,indent=2)+"\n")
    print(json.dumps({"passed":True,"cases":len(rows),"report":str(output/"cli-validation.json")}))
    return 0

if __name__ == "__main__":
    raise SystemExit(main())
