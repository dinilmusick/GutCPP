// Execute pinned GutTS parseJson/stringifyJson against real native C++17.
import {spawnSync} from 'node:child_process';
import {resolve,join} from 'node:path';
import {pathToFileURL} from 'node:url';
if(process.argv.length!==4)throw Error('Usage: verify-json-numbers.mjs <upstream-root> <native-runner>');
const upstream=resolve(process.argv[2]);
const runner=resolve(process.argv[3]);
const {parseJson,stringifyJson}=await import(pathToFileURL(join(upstream,'dist/json/json-concepts.js')).href);
const cases=[
 '0','-0','-0.0','0.1','0.10000000000000001','0.000001','0.0000001','1e-6','1e-7',
 '1e20','1e21','1e22','1.23e21','1.234e-8','1.7976931348623157e308',
 '5e-324','1e-324','1e-400','1e400','-1e400','1e9999','0e99999','-0e-99999',
 '9007199254740991','9007199254740992','9007199254740993',
 '[0.1,-0,1e400,1e-400,5e-324]',
 '[1.0000000000000002,1.005,2.675,-0.0]',
 '{"a":0.1,"b":[1e21,1e-7],"c":1e400}',
];
let seed=0x45c9f3a1;
function random(){seed^=seed<<13;seed^=seed>>>17;seed^=seed<<5;return (seed>>>0)/4294967296;}
for(let i=0;i<2500;i++){
 const sign=random()<.5?'-':'';
 const whole=1+Math.floor(random()*9);
 const fraction=String(Math.floor(random()*1e9)).padStart(9,'0');
 const exponent=Math.floor(random()*1300)-650;
 cases.push(sign+whole+'.'+fraction+'e'+exponent);
}
const native=spawnSync(runner,[],{input:cases.join('\n')+'\n',encoding:'utf8',maxBuffer:10*1024*1024,timeout:30000});
if(native.error||native.status!==0)throw Error('Native JSON failed: '+(native.stderr||native.error));
const lines=native.stdout.trim().split(/\r?\n/);
if(lines.length!==cases.length)throw Error('Native response count '+lines.length+'/'+cases.length);
let failures=0;
for(let i=0;i<cases.length;i++){
 const expected=stringifyJson(parseJson(cases[i]));
 if(lines[i]!==expected){
   failures++;
   if(failures<=12)console.error(JSON.stringify({input:cases[i],expected,actual:lines[i]}));
 }
}
console.log('Pinned GutTS C++17 numeric JSON differential '+(cases.length-failures)+'/'+cases.length+'; mismatches='+failures);
if(failures)process.exit(1);
