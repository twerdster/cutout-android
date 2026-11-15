'use strict';
// Updated 2026 client, adapted from the local desktop drawing interface.
const $ = id => document.getElementById(id);
const canvas = $('canvas'), context = canvas.getContext('2d', {willReadFrequently:true});
const controls = ['file','demo','density','distance','alpha','working','undo','clear','foreground','background'];
const colors = {foreground:'#12bf62', background:'#ef304b'};
let image = null, strokes = [], active = null, activePointer = null, kind = 'foreground', busy = false;
let worker = null, runId = 0, imageLoadId = 0;
function status(message, error=false) {
  $('status').textContent = message;
  $('status').classList.toggle('error', error);
}
function invalidate() {
  $('outputs').hidden = true;
  for (const id of ['cutout','mask']) $(id).removeAttribute('src');
  for (const id of ['download','download-mask']) $(id).removeAttribute('href');
  $('record').textContent = '';
}
function updateControls() {
  for (const id of controls) $(id).disabled = busy;
  $('cancel').disabled = !busy;
  $('run').disabled = busy || !image || !strokes.some(s => s.kind==='foreground') || !strokes.some(s => s.kind==='background');
}
function drawStroke(ctx, stroke, color) {
  if (!stroke.points.length) return;
  ctx.strokeStyle = ctx.fillStyle = color;
  ctx.lineWidth = 5;
  ctx.lineJoin = ctx.lineCap = 'round';
  const [first, ...rest] = stroke.points;
  ctx.beginPath(); ctx.moveTo(...first);
  for (const point of rest) ctx.lineTo(...point);
  ctx.stroke();
  if (!rest.length) {ctx.beginPath(); ctx.arc(...first,2.5,0,2*Math.PI); ctx.fill();}
}
function render() {
  context.fillStyle = 'white'; context.fillRect(0,0,canvas.width,canvas.height);
  if (image) context.drawImage(image,0,0,canvas.width,canvas.height);
  for (const stroke of strokes) drawStroke(context,stroke,colors[stroke.kind]);
  updateControls();
}
function point(event) {
  const box = canvas.getBoundingClientRect();
  return [Math.max(0,Math.min(canvas.width-1,Math.round((event.clientX-box.left)*canvas.width/box.width))),
    Math.max(0,Math.min(canvas.height-1,Math.round((event.clientY-box.top)*canvas.height/box.height)))];
}
canvas.addEventListener('pointerdown', event => {
  if (!image || busy || activePointer!==null) return;
  activePointer=event.pointerId;
  event.preventDefault(); canvas.setPointerCapture(event.pointerId); invalidate();
  active = {kind,points:[point(event)]}; strokes.push(active); render();
});
canvas.addEventListener('pointermove', event => {
  if (!active || busy || event.pointerId!==activePointer) return;
  event.preventDefault(); const next = point(event), last = active.points.at(-1);
  if (next[0]!==last[0] || next[1]!==last[1]) active.points.push(next);
  render();
});
for (const event of ['pointerup','pointercancel','lostpointercapture']) canvas.addEventListener(event, e => {
  if(e.pointerId===activePointer) {active=null; activePointer=null; updateControls();}
});
for (const name of ['foreground','background']) $(name).addEventListener('click', () => {
  kind = name;
  for (const other of ['foreground','background']) {
    $(other).classList.toggle('selected',other===kind);
    $(other).setAttribute('aria-pressed',String(other===kind));
  }
});
$('undo').addEventListener('click', () => {strokes.pop(); invalidate(); render(); status('Last stroke removed.');});
$('clear').addEventListener('click', () => {strokes=[]; invalidate(); render(); status('Scribbles cleared. Mark foreground and background.');});
async function decodeImage(blob) {
  if (blob.size > 20*1024*1024) throw Error('Choose an image smaller than 20 MB.');
  const url = URL.createObjectURL(blob);
  try {
    const next = new Image(); next.src=url; await next.decode();
    if (!next.naturalWidth || !next.naturalHeight || next.naturalWidth*next.naturalHeight > 40000000) throw Error('Choose an image with at most 40 million pixels.');
    return next;
  } finally {URL.revokeObjectURL(url);}
}
function setImage(next) {
  const scale = Math.min(1,1000/Math.max(next.naturalWidth,next.naturalHeight));
  canvas.width = Math.max(1,Math.round(next.naturalWidth*scale));
  canvas.height = Math.max(1,Math.round(next.naturalHeight*scale));
  image=next; strokes=[]; active=null; activePointer=null; invalidate(); render();
}
$('file').addEventListener('change', async event => {
  const file=event.target.files[0]; if (!file || busy) return;
  const load=++imageLoadId;
  try {const next=await decodeImage(file); if (load!==imageLoadId || busy) return; setImage(next); status('Image loaded. Draw foreground and background scribbles.');}
  catch(error) {if (load===imageLoadId) status(error.message || 'Could not decode this image.',true);}
  finally {event.target.value='';}
});
$('demo').addEventListener('click', async () => {
  const load=++imageLoadId;
  try {
    const [photo,seeds]=await Promise.all([fetch('coffee.png'),fetch('coffee-scribbles.json')]);
    if (!photo.ok || !seeds.ok) throw Error('The example files could not be loaded.');
    const [next,data]=await Promise.all([photo.blob().then(decodeImage),seeds.json()]);
    if (load!==imageLoadId || busy) return;
    setImage(next);
    strokes=['foreground','background'].flatMap(name=>data[name].map(points=>({kind:name,points})));
    render(); status('Coffee example loaded with demonstration scribbles. Run segmentation or edit the strokes.');
  } catch(error) {if (load===imageLoadId) status(error.message,true);}
});
for (const id of ['density','distance','alpha','working']) $(id).addEventListener('change', () => {invalidate(); if(image) status('Settings changed. Run again to update the result.');});
function makeBuffers() {
  const scratch=document.createElement('canvas'); scratch.width=canvas.width; scratch.height=canvas.height;
  const ctx=scratch.getContext('2d',{willReadFrequently:true});
  ctx.fillStyle='white'; ctx.fillRect(0,0,scratch.width,scratch.height);
  ctx.drawImage(image,0,0,scratch.width,scratch.height);
  const rgba=ctx.getImageData(0,0,scratch.width,scratch.height).data;
  const labels=new Uint8Array(scratch.width*scratch.height);
  for (const stroke of strokes) {
    ctx.clearRect(0,0,scratch.width,scratch.height); drawStroke(ctx,stroke,'white');
    const data=ctx.getImageData(0,0,scratch.width,scratch.height).data;
    for(let i=0;i<labels.length;i++) if(data[4*i+3]) labels[i]=stroke.kind==='foreground'?1:2;
  }
  if (!labels.includes(1) || !labels.includes(2)) throw Error('Both foreground and background labels are required. A later stroke may have replaced a class.');
  return {rgba:rgba.buffer, labels:labels.buffer};
}
function png(bytes, width, height) {
  const output=document.createElement('canvas'); output.width=width; output.height=height;
  output.getContext('2d').putImageData(new ImageData(new Uint8ClampedArray(bytes),width,height),0,0);
  return output.toDataURL('image/png');
}
function complete(data, options, width, height) {
  if (data.id!==runId || !busy) return;
  busy=false; updateControls();
  if (data.error) {
    worker.terminate(); worker=null;
    status(data.error,true); return;
  }
  const rgba=new Uint8Array(data.output), mask=new Uint8Array(data.mask);
  const maskRgba=new Uint8ClampedArray(width*height*4);
  let count=0;
  for(let i=0;i<mask.length;i++) {
    maskRgba[4*i]=maskRgba[4*i+1]=maskRgba[4*i+2]=mask[i]; maskRgba[4*i+3]=255;
    if(mask[i]) count++;
  }
  $('cutout').src=$('download').href=png(rgba,width,height);
  $('mask').src=$('download-mask').href=png(maskRgba,width,height);
  $('record').textContent=JSON.stringify({implementation:'Shared CutOut C++ core / WebAssembly',width,height,...options,
    foreground_pixels:count,elapsed_seconds:data.elapsed_seconds,scope:'One local execution; no reference-mask accuracy measurement.'},null,2);
  $('outputs').hidden=false;
  status(`Segmentation complete (${data.elapsed_seconds.toFixed(2)} s). Inspect the mask and cutout; results depend on the image and scribbles.`);
}
$('run').addEventListener('click', () => {
  if (busy || !image) return;
  try {
    const buffers=makeBuffers();
    const options={density:$('density').value,distance:$('distance').value,alpha:$('alpha').value,working:Number($('working').value)};
    const width=canvas.width,height=canvas.height;
    imageLoadId++; busy=true; active=null; activePointer=null; invalidate(); updateControls();
    status('Loading WebAssembly and running segmentation locally…');
    if (!worker) worker=new Worker('worker.js');
    const id=++runId;
    worker.onmessage=event=>complete(event.data,options,width,height);
    worker.onerror=()=>{
      if(id!==runId) return;
      worker.terminate(); worker=null;
      if(!busy) return;
      busy=false; updateControls();
      status('The browser worker stopped. Check that the Wasm files loaded, or try a smaller image.',true);
    };
    worker.postMessage({id,width,height,options,...buffers},[buffers.rgba,buffers.labels]);
  } catch(error) {busy=false; updateControls(); status(error.message,true);}
});
$('cancel').addEventListener('click', () => {
  runId++; if(worker) worker.terminate(); worker=null; busy=false; invalidate(); updateControls();
  status('Run canceled. Inputs are preserved; you can edit them and run again.');
});
render();
