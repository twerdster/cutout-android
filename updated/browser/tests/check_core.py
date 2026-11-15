"""Compare native and Wasm bridge outputs on fixed, identically rasterized inputs.

Requires Pillow only to decode the pinned photo and rasterize shared test labels.
"""
from pathlib import Path
import argparse
import hashlib
import json
import math
import subprocess
from PIL import Image, ImageDraw

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--native', type=Path, required=True)
parser.add_argument('--wasm-js', type=Path, required=True)
parser.add_argument('--output', type=Path, required=True)
args = parser.parse_args()
args.output.mkdir(parents=True, exist_ok=True)
repo = Path(__file__).resolve().parents[3]
photo = repo / 'updated/validation/fixtures/coffee.png'
assert hashlib.sha256(photo.read_bytes()).hexdigest() == json.loads((photo.parent/'PROVENANCE.json').read_text())['files'][0]['sha256']
rows = []
for name in ('ellipse','coffee'):
    if name == 'coffee':
        image = Image.open(photo).convert('RGBA')
        seed_data = json.loads((photo.parent/'coffee-scribbles.json').read_text())
    else:
        image = Image.new('RGBA',(80,64),(40,62,85,255))
        ImageDraw.Draw(image).ellipse((19,10,62,54),fill=(210,116,46,255))
        seed_data={'foreground':[[[36,25],[47,37]]],'background':[[[3,3],[75,3],[75,60],[3,60],[3,3]]]}
    labels=Image.new('L',image.size)
    draw=ImageDraw.Draw(labels)
    for label,kind in ((1,'foreground'),(2,'background')):
        for points in seed_data[kind]:
            points=[tuple(p) for p in points]
            draw.line(points,fill=label,width=5)
            for x,y in points: draw.ellipse((x-2,y-2,x+2,y+2),fill=label)
    image_path=args.output/(name+'.input.rgba'); image_path.write_bytes(image.tobytes())
    labels_path=args.output/(name+'.labels'); labels_path.write_bytes(labels.tobytes())
    for figtree in (0,1):
        for levelset in (0,1):
            for refine in (0,1):
                case=f'{name}-{figtree}-{levelset}-{refine}'
                common=[str(image_path.resolve()),str(labels_path.resolve()),str(image.width),str(image.height),str(figtree),str(levelset),str(refine),'400']
                native=args.output/(case+'.native'); wasm=args.output/(case+'.wasm')
                subprocess.run([str(args.native.resolve()),*common,str(native.resolve())],check=True,capture_output=True)
                subprocess.run(['node',str(Path(__file__).with_name('run_wasm.cjs')),str(args.wasm_js.resolve()),*common,str(wasm.resolve())],check=True,capture_output=True)
                a=Path(str(native)+'.mask').read_bytes(); b=Path(str(wasm)+'.mask').read_bytes()
                ar=Path(str(native)+'.rgba').read_bytes(); br=Path(str(wasm)+'.rgba').read_bytes()
                assert len(a)==len(b)==image.width*image.height
                assert set(a)<={0,255} and set(b)<={0,255}
                assert 0<sum(x==255 for x in b)<len(b)
                union=sum(bool(x or y) for x,y in zip(a,b)); intersection=sum(bool(x and y) for x,y in zip(a,b))
                iou=intersection/union if union else 1
                assert iou>=0.995,(case,'native/Wasm masks differ',iou)
                seeds=labels.tobytes()
                assert all((label!=1 or b[i]==255) and (label!=2 or b[i]==0) for i,label in enumerate(seeds))
                input_bytes=image.tobytes()
                assert all(br[4*i:4*i+3]==input_bytes[4*i:4*i+3] for i in range(len(b)))
                assert all((label!=1 or br[4*i+3]==255) and (label!=2 or br[4*i+3]==0) for i,label in enumerate(seeds))
                if not refine: assert br[3::4]==b
                alpha_diff=[abs(x-y) for x,y in zip(ar[3::4],br[3::4])]
                rows.append({'case':case,'density':'figtree' if figtree else 'histogram','distance':'levelset' if levelset else 'pixel','alpha':'refined' if refine else 'hard','width':image.width,'height':image.height,'working_limit':400,'binary_mask_differing_pixels':sum(x!=y for x,y in zip(a,b)),'mask_iou_native_vs_wasm':iou,'alpha_max_byte_difference':max(alpha_diff),'alpha_mean_absolute_byte_difference':sum(alpha_diff)/len(alpha_diff),'wasm_seed_labels_and_rgb_preserved':True,'wasm_mask_sha256':hashlib.sha256(b).hexdigest(),'wasm_rgba_sha256':hashlib.sha256(br).hexdigest()})
                print(case, 'IoU',iou,'alpha max difference',max(alpha_diff),flush=True)
rejections=[]
image_path=args.output/'coffee.input.rgba'
for case,width,height,working,seed_bytes,message in (
    ('missing-seeds',600,400,400,bytes(240000),'both foreground and background'),
    ('one-class',600,400,400,bytes([1])*240000,'both foreground and background'),
    ('reduced-seed-collision',600,400,400,bytes([1,2])+bytes(239998),'collide after reduction'),
    ('invalid-working-limit',600,400,401,(args.output/'coffee.labels').read_bytes(),'400 or 800'),
    ('zero-width',0,400,400,(args.output/'coffee.labels').read_bytes(),'1..1,000,000'),
    ('over-pixel-limit',1001,1000,400,(args.output/'coffee.labels').read_bytes(),'1..1,000,000'),
):
    seed_path=args.output/(case+'.labels');seed_path.write_bytes(seed_bytes)
    common=[str(image_path.resolve()),str(seed_path.resolve()),str(width),str(height),'0','0','0',str(working),str((args.output/case).resolve())]
    # The native driver checks file dimensions before the C ABI, so the two
    # malformed-dimension cases exercise Wasm only. The other errors test both.
    implementations=['wasm'] if case in ('zero-width','over-pixel-limit') else ['native','wasm']
    for implementation in implementations:
        command=([str(args.native.resolve())] if implementation=='native' else
                 ['node',str(Path(__file__).with_name('run_wasm.cjs')),str(args.wasm_js.resolve())])
        result=subprocess.run([*command,*common],capture_output=True,text=True)
        assert result.returncode!=0 and message in result.stderr,(case,implementation,result.stderr)
    rejections.append({'case':case,'implementations':implementations,'expected_error_fragment':message})
result={'native_opencv':subprocess.check_output([str(args.native.resolve()),'--version'],text=True).strip(),'wasm_opencv':'4.12.0','node':subprocess.check_output(['node','--version'],text=True).strip(),'cases':rows,'passed':len(rows),'expected_rejections':rejections,'scope':'Same RGBA/label inputs through new native and Wasm bridge builds; masks compare at IoU >=0.995. Not a historical APK test or accuracy benchmark.'}
(args.output/'core-comparison.json').write_text(json.dumps(result,indent=2)+'\n')
