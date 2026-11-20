// Cross-language differential for the ACTUAL pinned GutTS Ettin head math.
// Both JS and C++ consume the same deterministic Float32 tensor values; this
// does not claim inference by the upstream transformer backbone.
import {spawnSync} from 'node:child_process';
import {pathToFileURL} from 'node:url';
import {join,resolve} from 'node:path';
if(process.argv.length!==4){console.error('Usage: verify-ettin-head.mjs <upstream-root> <runner>');process.exit(2);}
const upstream=resolve(process.argv[2]), runner=resolve(process.argv[3]);
const {computeEttinHeadScores}=await import(pathToFileURL(join(upstream,'dist/models/runtimes/ettin-head.js')).href);
let seed=0x19D11015;
const random=()=>{seed=(Math.imul(seed,1664525)+1013904223)>>>0;return seed/4294967296;};
const f32=(count)=>Array.from({length:count},()=>Math.fround((random()*2-1)*.8));
const cases=[];
for(const size of [1,2,3,7,16]){
 for(const batch of [1,2,3]){
  const seqLen=1+(size%3);
  cases.push({hidden:{dims:[batch,seqLen,size],data:f32(batch*seqLen*size)},
    head:{w1:f32(size*size),lnWeight:f32(size),lnBias:f32(size),w2:f32(size),b2:f32(1)}});
 }
}
const input=cases.map(c=>JSON.stringify(c)).join('\n')+'\n';
const native=spawnSync(runner,[],{input,encoding:'utf8',maxBuffer:8*1024*1024,timeout:20000});
if(native.status!==0||native.error){console.error('C++ runner failed',native.stderr,native.error);process.exit(1);}
const lines=native.stdout.trim().split(/\r?\n/);
if(lines.length!==cases.length)throw new Error(`Ettin native result count mismatch ${lines.length}/${cases.length}`);
let mismatches=0,peakError=0;
for(let i=0;i<cases.length;++i){
 const c=cases[i],head=Object.fromEntries(Object.entries(c.head).map(([k,v])=>[k,Float32Array.from(v)]));
 const hidden={dims:c.hidden.dims,data:Float32Array.from(c.hidden.data)};
 const expected=computeEttinHeadScores(hidden,head);
 const actual=JSON.parse(lines[i]);
 if(!Array.isArray(actual)||actual.length!==expected.length){++mismatches;continue;}
 for(let j=0;j<expected.length;++j){
   const delta=Math.abs(expected[j]-actual[j]);peakError=Math.max(peakError,delta);
   if(!Number.isFinite(actual[j])||delta>2e-6){++mismatches;console.error('Ettin cross-language mismatch',i,j,expected[j],actual[j],delta);}
 }
}
console.log(`Actual pinned GutTS Ettin score parity: ${cases.length} tensor fixtures, max absolute error ${peakError}, ${mismatches} mismatches`);
if(mismatches)process.exit(1);
