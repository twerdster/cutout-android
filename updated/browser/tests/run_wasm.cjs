// Isolated Node execution of the same exported Wasm bridge used by the worker.
const fs = require('node:fs');
const path = require('node:path');
const [modulePath, imagePath, labelsPath, width, height, figtree, levelset, refine, working, outputPath] = process.argv.slice(2);
(async () => {
  const create = require(path.resolve(modulePath));
  const module = await create();
  const rgba=fs.readFileSync(imagePath), labels=fs.readFileSync(labelsPath);
  const pointers=[];
  try {
    for(const count of [rgba.length,labels.length,rgba.length,labels.length]) pointers.push(module._malloc(count));
    module.HEAPU8.set(rgba,pointers[0]); module.HEAPU8.set(labels,pointers[1]);
    const error=module._cutout_segment(pointers[0],pointers[1],+width,+height,+figtree,+levelset,+refine,+working,pointers[2],pointers[3]);
    if(error) throw Error(module.UTF8ToString(module._cutout_error()));
    fs.writeFileSync(outputPath+'.rgba',module.HEAPU8.slice(pointers[2],pointers[2]+rgba.length));
    fs.writeFileSync(outputPath+'.mask',module.HEAPU8.slice(pointers[3],pointers[3]+labels.length));
  } finally {for(const pointer of pointers) module._free(pointer);}
})().catch(error=>{console.error(error.message);process.exitCode=1;});
