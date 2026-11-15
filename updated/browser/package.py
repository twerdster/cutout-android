"""Package only the static browser app and its required public credit files."""
from pathlib import Path
import shutil
import sys
import tarfile
import json
import hashlib
import re

here = Path(__file__).resolve().parent
repo = here.parents[1]
build, output, opencv, emscripten_license = (Path(p).resolve() for p in sys.argv[1:])
marker = output / '.cutout-browser-package'
if output.exists() and any(output.iterdir()):
    if not marker.is_file():
        raise SystemExit('Output is not an identified CutOut package; choose a new empty directory.')
    shutil.rmtree(output)
output.mkdir(parents=True, exist_ok=True)
marker.write_text('Generated CutOut static browser package\n')
for name in ('index.html', 'styles.css', 'app.js', 'worker.js', 'credits.txt'):
    shutil.copy2(here / name, output / name)
for name in ('cutout_wasm.js', 'cutout_wasm.wasm'):
    data = (build / name).read_bytes()
    private_prefixes = {str(repo), str(build), str(opencv), str(emscripten_license.parent),
                        str(Path.home()), '/Users/', '/Volumes/', '/home/runner/'}
    if any(prefix != '/' and prefix.encode() in data for prefix in private_prefixes):
        raise SystemExit('Build paths remain in the generated binary/runtime; rebuild with prefix maps.')
    shutil.copy2(build / name, output / name)
shutil.copy2(repo / 'updated/validation/fixtures/coffee.png', output / 'coffee.png')
shutil.copy2(repo / 'updated/validation/fixtures/coffee-scribbles.json', output / 'coffee-scribbles.json')
shutil.copytree(repo / 'docs/licenses', output / 'licenses', dirs_exist_ok=True)
shutil.copy2(opencv / 'LICENSE', output / 'licenses/OpenCV-LICENSE.txt')
shutil.copy2(opencv / '3rdparty/zlib/LICENSE', output / 'licenses/zlib-LICENSE.txt')
shutil.copy2(emscripten_license, output / 'licenses/Emscripten-LICENSE.txt')
runtime_notices = {'musl-COPYRIGHT.txt':'libc/musl/COPYRIGHT',
                   'libcxx-LICENSE.txt':'libcxx/LICENSE.TXT',
                   'libcxxabi-LICENSE.txt':'libcxxabi/LICENSE.TXT',
                   'compiler-rt-LICENSE.txt':'compiler-rt/LICENSE.TXT'}
for name, path in runtime_notices.items():
    shutil.copy2(emscripten_license.parent/'system/lib'/path, output/'licenses'/name)
shutil.copy2(here / 'licenses.html', output / 'licenses/index.html')
def public_link(match):
    target = match[1]
    if target.startswith(('https:', 'http:', '#')):
        return match[0]
    path, _, fragment = target.partition('#')
    relative = (here / path).resolve().relative_to(repo)
    return '](' + 'https://github.com/twerdster/cutout-android/blob/main/' + str(relative) + ('#'+fragment if fragment else '') + ')'
(output / 'browser-guide.txt').write_text(re.sub(r'\]\(([^)]+)\)', public_link, (here/'README.md').read_text()))
source_files = []
for directory in ('updated/native', 'updated/browser', 'updated/validation/fixtures', 'docs/licenses'):
    source_files.extend(p for p in (repo / directory).rglob('*')
                        if p.is_file() and '__pycache__' not in p.parts and p.name != '.DS_Store')
source_files.append(repo / 'docs/ATTRIBUTION.md')
with tarfile.open(output / 'source.tar.gz', 'w:gz') as archive:
    def generic_metadata(info):
        info.uid=info.gid=0; info.uname=info.gname=''; info.mtime=0; info.pax_headers={}
        return info
    for path in sorted(source_files):
        archive.add(path, arcname=str(path.relative_to(repo)), filter=generic_metadata)
(output / '.nojekyll').write_text('')
manifest = {str(p.relative_to(output)):hashlib.sha256(p.read_bytes()).hexdigest()
            for p in sorted(output.rglob('*')) if p.is_file() and p.name != 'package-hashes.json'}
(output / 'package-hashes.json').write_text(json.dumps(manifest, indent=2) + '\n')
print(f'Static browser package: {output}')
