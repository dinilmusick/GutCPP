// Compare actual pinned GutTS runtime dispatch against native C++17 execution.
// This is not a mocked fixture: GitHub Actions compiles the immutable TS source
// and passes its absolute directory to this script.
import {spawnSync} from 'node:child_process';
import {pathToFileURL} from 'node:url';
import {resolve,join} from 'node:path';

if(process.argv.length!==4){
 console.error('Usage: node scripts/verify-upstream-differential.mjs <upstream-root> <gutcpp-runner>');
 process.exit(2);
}
const upstream=resolve(process.argv[2]);
const runner=resolve(process.argv[3]);
const {executeGuttsApi,guttsApiDefinitions}=await import(pathToFileURL(join(upstream,'dist/api/index.js')).href);
const cases=[];
const add=(apiId,label,input)=>cases.push({apiId,label,input});
const group=(section,name,values)=>values.forEach(([label,input])=>add(`gutts.${section}.${name}`,label,input));

group('arrays','compact',[
 ['mixed',{values:[null,0,false,'',1,null,2]}],['empty',{values:[]}],
 ['objects',{values:[{a:1},null,{},'x']}],
 ['empty-containers-are-truthy',{values:[[],{},false,'',0]}],
 ['negative-zero',{values:[-0,0,-1,1]}]]);
group('arrays','unique',[
 ['numbers',{values:[1,2,1,3,2,0]}],['strings',{values:['x','y','x','']}],
 ['empty',{values:[]}],
 ['distinct-object-identities',{values:[{a:1},{a:1},1,1]}],
 ['distinct-array-identities',{values:[[1],[1],0,0]}],
 ['boolean-and-null',{values:[true,false,true,null,null,false]}]]);
group('arrays','chunk',[
 ['size-two',{values:[1,2,3,4,5],size:2}],['size-one',{values:['a','b'],size:1}],
 ['oversize',{values:[1,2],size:10}],['size-zero',{values:[1],size:0}],['negative',{values:[1],size:-1}]]);
group('arrays','flatten',[
 ['mixed',{values:[[1,2],[3],4]}],['nested',{values:[[[1]],2,[3]]}],['empty',{values:[]}]]);
group('arrays','last',[
 ['present',{values:[1,2,3]}],['empty',{values:[]}],['null',{values:[1,null]}]]);
group('json','isJsonValue',[
 ['object',{value:{a:[1,true,null],b:'x'}}],['null',{value:null}],['array',{value:[]}]]);
group('json','parseJson',[
 ['object',{text:'{"a":1,"b":[true,null]}'}],['scalar',{text:'"hello"'}],['invalid',{text:'{'}]]);
group('json','tryParseJson',[
 ['valid',{text:'[1,2]'}],['invalid',{text:'{'}]]);
group('json','stringifyJson',[
 ['scalar',{value:'hello'}],['array',{value:[true,null,2]}],['pretty',{value:{a:1},space:2}]]);
group('numbers','clamp',[
 ['inside',{value:3,min:0,max:5}],['below',{value:-2,min:0,max:5}],
 ['above',{value:10,min:0,max:5}],['invalid',{value:1,min:9,max:2}]]);
group('numbers','inRange',[
 ['inside',{value:2,min:0,max:3}],['upper',{value:3,min:0,max:3}],
 ['lower',{value:0,min:0,max:3}],['invalid',{value:1,min:9,max:2}]]);
group('numbers','roundTo',[
 ['two-decimals',{value:1.2345,decimals:2}],['negative',{value:-2.345,decimals:1}],
 ['default',{value:4.6}],['invalid',{value:1.1,decimals:-1}]]);
group('numbers','isFiniteNumber',[
 ['number',{value:4}],['null',{value:null}],['string',{value:'4'}]]);
group('numbers','sum',[
 ['mixed',{values:[1.25,2.5,-3]}],['empty',{values:[]}],['integers',{values:[0,4,6]}]]);
group('objects','hasOwn',[
 ['present',{value:{a:1},key:'a'}],['missing',{value:{a:1},key:'b'}]]);
group('objects','pick',[
 ['two',{value:{a:1,b:2,c:3},keys:['a','c']}],['missing',{value:{x:1},keys:['z']}]]);
group('objects','omit',[
 ['one',{value:{a:1,b:2},keys:['a']}],['empty',{value:{a:1},keys:[]}]]);
group('objects','isPlainObject',[
 ['object',{value:{a:1}}],['array',{value:[1,2]}],['null',{value:null}]]);
group('strings','isBlank',[
 ['ascii',{value:' \t'}],['word',{value:'x'}],['unicode',{value:'\u2003'}]]);
group('strings','isNonBlank',[
 ['ascii',{value:' ' }],['text',{value:'abc'}]]);
group('strings','isEmail',[
 ['valid',{value:'abc@example.com'}],['invalid',{value:'not-email'}],['subdomain',{value:'a+b@sub.example.org'}]]);
group('strings','startsWithAny',[
 ['present',{value:'GutCPP',prefixes:['gut','Gut']}],['missing',{value:'abc',prefixes:['b','c']}],
 ['empty',{value:'text',prefixes:[]}]]);
group('strings','endsWithAny',[
 ['present',{value:'GutCPP',suffixes:['CPP','JS']}],['missing',{value:'abc',suffixes:['b','a']}]]);
group('strings','capitalize',[
 ['ascii',{value:'hello WORLD'}],['empty',{value:''}],['upper',{value:'HELLO'}]]);
group('strings','camelCase',[
 ['ascii',{value:'hello world'}],['kebab',{value:'my-name'}],['snake',{value:'user_name'}]]);
group('strings','kebabCase',[
 ['ascii',{value:'hello world'}],['camel',{value:'userName'}]]);
group('strings','snakeCase',[
 ['ascii',{value:'hello world'}],['camel',{value:'userName'}]]);
group('strings','truncate',[
 ['short',{value:'hello',maxLength:10}],['long',{value:'abcdefghij',maxLength:6}],
 ['explicit',{value:'abcdefgh',maxLength:6,suffix:'...'}],['invalid',{value:'a',maxLength:-1}]]);
group('strings','truncateWords',[
 ['short',{value:'one two',maxWords:3}],['long',{value:'one two three four',maxWords:2}],
 ['invalid',{value:'one two',maxWords:-1}]]);
group('urls','isUrl',[
 ['https',{value:'https://example.com/path'}],['relative',{value:'/relative'}],['scheme',{value:'http://127.0.0.1:8080'}]]);
group('urls','joinUrl',[
 ['multiple',{base:'https://example.com///',parts:['/a/','//b/','']}],['empty',{base:'/',parts:['a','b']}]]);
group('urls','getQueryParam',[
 ['present',{url:'https://example.com/?x=hello+world',key:'x'}],['missing',{url:'https://example.com/',key:'x'}],
 ['duplicate',{url:'https://example.com/?a=1&a=2',key:'a'}]]);
group('urls','setQueryParam',[
 ['append',{url:'https://example.com/?a=1',key:'b',value:'hello world'}],
 ['replace',{url:'https://example.com/?a=1',key:'a',value:'2'}]]);

const supported=guttsApiDefinitions.filter(api=>['arrays','json','numbers','objects','strings','urls'].includes(api.category));
const upstreamIds=new Set(supported.map(api=>api.id));
const caseIds=new Set(cases.map(c=>c.apiId));
if(upstreamIds.size!==33||[...upstreamIds].some(id=>!caseIds.has(id))||[...caseIds].some(id=>!upstreamIds.has(id))){
 console.error('Differential coverage does not match the exact published GutTS utility registry');process.exit(1);
}
// The canonical TS registry includes five metadata-only model operations.
// Compare all 38 contract IDs, parameter shapes, output types and semantics
// against the actual pinned TS source rather than a handwritten list.
const expectedRegistry=guttsApiDefinitions.map(api=>({
 id:api.id,name:api.name,category:api.category,
 parameters:api.parameters.map(p=>({name:p.name,required:p.required,jsonType:p.schema.type??'any'})),
 outputType:api.output.type??'any',
 deterministic:api.semantics.deterministic,pure:api.semantics.pure,sideEffects:api.semantics.sideEffects,
}));
if(expectedRegistry.length!==38){throw new Error('Pinned TS model/utility public API registry changed');}
const registryNative=spawnSync(runner,[],{input:'{"registry":true}\n',encoding:'utf8',maxBuffer:1024*1024,timeout:10000});
if(registryNative.status!==0||registryNative.error){throw new Error('C++ registry probe failed');}
const actualRegistry=JSON.parse(registryNative.stdout.trim()).registry;
if(JSON.stringify(expectedRegistry)!==JSON.stringify(actualRegistry)){
 for(let idx=0;idx<Math.max(expectedRegistry.length,actualRegistry?.length??0);idx++){
  if(JSON.stringify(expectedRegistry[idx])!==JSON.stringify(actualRegistry?.[idx])){
   console.error('Registry mismatch',JSON.stringify({index:idx,expected:expectedRegistry[idx],actual:actualRegistry?.[idx]}));
  }
 }
 throw new Error('Pinned GutTS/C++ model and utility API registry mismatch');
}
console.log(`Pinned GutTS public API registry parity: ${expectedRegistry.length}/${expectedRegistry.length} definitions`);

const requests=cases.map(c=>JSON.stringify({apiId:c.apiId,input:c.input})).join('\n')+'\n';
const native=spawnSync(runner,[],{input:requests,encoding:'utf8',maxBuffer:16*1024*1024,timeout:30000});
if(native.error||native.status!==0){
 console.error('Native differential runner failed',native.status,native.error,native.stderr);process.exit(1);
}
const raw=native.stdout.trim().split(/\r?\n/);
if(raw.length!==cases.length){console.error('Native response count mismatch',raw.length,cases.length);process.exit(1);}
const nativeResults=raw.map(line=>JSON.parse(line));
const stable=value=>{
 if(Array.isArray(value))return value.map(stable);
 if(value&&typeof value==='object')return Object.fromEntries(Object.keys(value).sort().map(key=>[key,stable(value[key])]));
 return value;
};
const equal=(a,b)=>JSON.stringify(stable(a))===JSON.stringify(stable(b));
let failures=0;
for(let i=0;i<cases.length;i++){
 const c=cases[i];let expected;
 try{
   const value=await executeGuttsApi(c.apiId,c.input);
   expected=value===undefined?{ok:true,defined:false}:{ok:true,defined:true,value};
 }catch(error){expected={ok:false,error:error instanceof Error?error.message:String(error)};}
 const actual=nativeResults[i];
 const matches=expected.ok===actual.ok && (!expected.ok ||
   (expected.defined===actual.defined && (!expected.defined || equal(expected.value,actual.value))));
 if(!matches){
   ++failures;
   console.error(JSON.stringify({case:c.apiId+':'+c.label,input:c.input,expected,actual}));
 }
}
console.log(`Actual pinned GutTS vs C++ executable differential: ${cases.length-failures}/${cases.length} identical results; ${upstreamIds.size} API IDs; ${failures} mismatches.`);
if(failures)process.exit(1);
