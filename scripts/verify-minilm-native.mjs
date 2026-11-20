// Actual compiled pinned GutTS MiniLM model inference vs native C++17 ONNX.
import {spawnSync} from 'node:child_process';
import {resolve,join} from 'node:path';
import {pathToFileURL} from 'node:url';
if(process.argv.length!==6)throw Error('usage: verify-minilm-native.mjs UPSTREAM_DIR NATIVE_RUNNER ONNX_MODEL TOKENIZER_JSON');
const [upstream,runner,model,tokenizer]=process.argv.slice(2).map(resolve);
const {miniLMRuntime}=await import(pathToFileURL(join(upstream,'dist/models/runtimes/transformers.js')).href);
const cases=[
 ['A cat is sitting on the sofa','A cat is sitting on the sofa'],
 ['A cat is sitting on the sofa','A kitten is resting on a couch'],
 ['A cat is sitting on the sofa','Quantum mechanics in black holes'],
 ['The weather is sunny today.','There are no clouds in the bright sky.'],
 ['An angry dog barks at strangers.','A canine is loudly barking.'],
 ['The main server returned an HTTP 500 error.','An internal server error occurred.'],
 ['This song has a fast tempo in 6/8 meter.','The music has an energetic compound rhythm.'],
 ['Pune is in Maharashtra.','Tokyo is in Japan.'],
 ['A doctor examines a patient.','A clinician checks someone at the hospital.'],
 ['Convert an array to a comma separated string','Make a CSV row from a list'],
 ['How do I learn TypeScript?','What is the best way to practice coding in TS?'],
 ['Hello world!','Goodbye universe!'],
];
const input=cases.map(([left,right])=>JSON.stringify({left,right})).join('\n')+'\n';
const native=spawnSync(runner,[model,tokenizer],{input,encoding:'utf8',maxBuffer:4*1024*1024,timeout:90000});
if(native.error||native.status!==0)throw Error(`Native C++ model inference failed: ${native.stderr||native.error}`);
const lines=native.stdout.trim().split(/\r?\n/);
if(lines.length!==cases.length)throw Error(`Native output count wrong: ${lines.length}/${cases.length}`);
let runtime;
try{
 runtime=await miniLMRuntime.load();
 let failures=0;
 for(let i=0;i<cases.length;i++){
  const [left,right]=cases[i];
  const expected=await runtime.similarity({left,right});
  const actual=JSON.parse(lines[i]);
  const delta=Math.abs(expected-actual);
  if(!Number.isFinite(expected)||!Number.isFinite(actual)||delta>0.025){
    failures++;
    if(failures<=10)console.error(JSON.stringify({pair:[left,right],reference:expected,native:actual,delta}));
  }
 }
 console.log(`Real compiled GutTS MiniLM vs native C++ ONNX: ${cases.length-failures}/${cases.length} within 0.025 cosine tolerance`);
 if(failures)process.exitCode=1;
}finally{if(runtime)await miniLMRuntime.unload(runtime);}
