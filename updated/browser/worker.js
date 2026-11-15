'use strict';
// Runs one segmentation at a time in this worker; Cancel terminates it.
importScripts('cutout_wasm.js');
const ready = createCutout({locateFile: name => new URL(name, self.location.href).href});
onmessage = async event => {
  const {id, width, height, rgba, labels, options} = event.data;
  const pointers = [];
  let module;
  try {
    module = await ready;
    const count = width * height;
    if (!Number.isInteger(width) || !Number.isInteger(height) || count < 1 || count > 1000000 || rgba.byteLength !== count * 4 || labels.byteLength !== count) throw Error('Invalid image buffers.');
    for (const bytes of [count * 4, count, count * 4, count]) {
      const pointer = module._malloc(bytes);
      if (!pointer) throw Error('Browser memory allocation failed. Try a smaller image.');
      pointers.push(pointer);
    }
    module.HEAPU8.set(new Uint8Array(rgba), pointers[0]);
    module.HEAPU8.set(new Uint8Array(labels), pointers[1]);
    const started = performance.now();
    const failed = module._cutout_segment(pointers[0], pointers[1], width, height,
      options.density === 'figtree', options.distance === 'levelset', options.alpha === 'refined',
      options.working, pointers[2], pointers[3]);
    if (failed) throw Error(module.UTF8ToString(module._cutout_error()));
    const output = module.HEAPU8.slice(pointers[2], pointers[2] + count * 4);
    const mask = module.HEAPU8.slice(pointers[3], pointers[3] + count);
    postMessage({id, output:output.buffer, mask:mask.buffer, elapsed_seconds:(performance.now()-started)/1000}, [output.buffer, mask.buffer]);
  } catch (error) { postMessage({id, error:error.message || String(error)}); }
  finally {
    // A trap may abort the module; the page replaces the worker after any error.
    if (module) for (const pointer of pointers) {try {module._free(pointer);} catch {break;}}
  }
};
