const fs=require('node:fs/promises'), assert=require('node:assert/strict');
const {chromium}=require('playwright');
const path=require('node:path'), root=path.resolve(process.argv[3]||'build/browser-qa');
const base=process.argv[2]||'http://127.0.0.1:8774/';
const fixture=path.resolve(__dirname,'../../validation/fixtures/coffee.png');
(async()=>{
 await fs.mkdir(root,{recursive:true});
 const browser=await chromium.launch({headless:true,...(process.env.BROWSER_CHANNEL?{channel:process.env.BROWSER_CHANNEL}:{})});
 const context=await browser.newContext({viewport:{width:1440,height:1100}}), external=[], errors=[];
 await context.route('**/*', route=>{const u=route.request().url();if(u.startsWith(base)||u.startsWith('data:')||u.startsWith('blob:'))return route.continue();external.push(u);return route.abort();});
 const page=await context.newPage();page.on('pageerror',e=>errors.push(e.message));
 // Wait for the task-owned local server (or a newly deployed static endpoint).
 let connected=false;
 for(let attempt=0;attempt<20;attempt++) {
  try {await page.goto(base);connected=true;break;} catch(error) {if(attempt===19) throw error;}
 }
 assert(connected);
 await page.locator('#demo').click();await page.waitForFunction(()=>document.querySelector('#status').textContent.startsWith('Coffee example loaded'));
 const cases=[];
 async function run(){await page.locator('#run').click();await page.waitForFunction(()=>!document.querySelector('#outputs').hidden||document.querySelector('#status').classList.contains('error'),{timeout:120000});const text=await page.locator('#status').innerText();assert(!await page.locator('#status').evaluate(e=>e.classList.contains('error')),text);await page.locator('#cutout').evaluate(im=>im.decode());const record=JSON.parse(await page.locator('#record').textContent());assert(record.foreground_pixels>0&&record.foreground_pixels<record.width*record.height);assert(record.elapsed_seconds>=0);return record;}
 for(const density of ['figtree','histogram'])for(const distance of ['pixel','levelset'])for(const alpha of ['hard','refined']){
  await page.locator('#density').selectOption(density);await page.locator('#distance').selectOption(distance);await page.locator('#alpha').selectOption(alpha);assert(await page.locator('#outputs').isHidden());
  const record=await run();cases.push(record);
  if(density==='figtree'&&distance==='levelset'&&alpha==='refined'){
   await page.screenshot({path:root+'/browser-1440.png',fullPage:true});
   for(const id of ['cutout','mask'])await fs.writeFile(root+'/browser-coffee-'+id+'.png',Buffer.from((await page.locator('#'+id).getAttribute('src')).split(',')[1],'base64'));
  }
 }
 await page.locator('#alpha').selectOption('hard');assert(await page.locator('#outputs').isHidden());
 await page.locator('#run').click();await page.locator('#cancel').click();assert(await page.locator('#outputs').isHidden());assert.match(await page.locator('#status').innerText(),/^Run canceled/);assert(await page.locator('#run').isEnabled());await run();
 // Real mouse strokes; later crossing correction and undo invalidate the result.
 await page.locator('#background').click();let box=await page.locator('#canvas').boundingBox();await page.mouse.move(box.x+box.width*.4,box.y+box.height*.35);await page.mouse.down();await page.mouse.move(box.x+box.width*.5,box.y+box.height*.45,{steps:10});await page.mouse.up();assert(await page.locator('#outputs').isHidden());await run();await page.locator('#undo').click();assert(await page.locator('#outputs').isHidden());
 await page.locator('#clear').click();assert(await page.locator('#run').isDisabled());
 await page.locator('#file').setInputFiles(fixture);await page.waitForFunction(()=>document.querySelector('#status').textContent.startsWith('Image loaded'));
 await page.locator('#foreground').click();box=await page.locator('#canvas').boundingBox();await page.mouse.click(box.x+box.width*.48,box.y+box.height*.3);await page.locator('#background').click();await page.mouse.click(box.x+box.width*.05,box.y+box.height*.9);await run();
 for(const [id,name] of [['download','downloaded-cutout.png'],['download-mask','downloaded-mask.png']]){const [download]=await Promise.all([page.waitForEvent('download'),page.locator('#'+id).click()]);await download.saveAs(path.join(root,name));}
 const layouts=[];
 for(const width of [390,1440]){await page.setViewportSize({width,height:1100});await page.locator('#demo').click();await page.waitForFunction(()=>document.querySelector('#status').textContent.startsWith('Coffee example loaded'));await page.locator('#density').selectOption('figtree');await page.locator('#distance').selectOption('levelset');await page.locator('#alpha').selectOption('refined');await run();const overflow=await page.evaluate(()=>document.documentElement.scrollWidth>innerWidth);assert(!overflow);layouts.push({width,overflow});await page.screenshot({path:root+'/browser-'+width+'.png',fullPage:true});}
 assert.equal(external.length,0,JSON.stringify(external));assert.equal(errors.length,0,JSON.stringify(errors));
 await fs.writeFile(root+'/browser-qa.json',JSON.stringify({browser:await browser.version(),operator_cases:cases,checks:['eight coffee operator combinations','actual local image upload','single-point and crossing pointer strokes','undo/clear','stale result invalidation','cancel and fresh-worker recovery','both PNG downloads'],layouts,external_requests:external,page_errors:errors},null,2));
 await browser.close();console.log('Browser checks passed',cases.length,layouts);
})().catch(error=>{console.error(error);process.exitCode=1;});
