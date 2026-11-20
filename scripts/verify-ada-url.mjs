// Compare native Ada WHATWG parsing/query behavior against ACTUAL compiled
// pinned GutTS url-operations, not a handwritten JavaScript approximation.
import {pathToFileURL} from 'node:url';
import {join,resolve} from 'node:path';
import {spawnSync} from 'node:child_process';
const upstream=resolve(process.argv[2]??'');
const runner=resolve(process.argv[3]??'');
const {isUrl,getQueryParam,setQueryParam}=await import(pathToFileURL(join(upstream,'dist/urls/url-operations.js')).href);
const urls=[
 'https://example.com','https://EXAMPLE.com:443/a/../b?x=hello+world&x=two#frag',
 'mailto:hello@example.com','data:text/plain,hello','ftp://example.org/file',
 'http://[::1]:8080/a?x=1','https://user:pass@example.com/a',
 'https://xn--bcher-kva.example/?x=%E2%9C%93','https://example.com/?x=1&x=2',
 'https://example.com/','file:///tmp/example','http://hello world','relative/path',
 'http://','https://example.com/?q=%2B&empty=','https://example.com/?a.b=7',
 'https://example.com/?x=one%20two','https://example.com/#hash',
 'https://example.com/?x=%3A%2F%40','https://example.com/?x=not+blank',
 'https://example.com/?x=', 'https://example.com/?%E2%9C%93=yes',
];
const cases=[];
for(const url of urls){
 cases.push({op:'isUrl',url});
 if(isUrl(url)){
  for(const key of ['x','q','missing','a.b','✓'])cases.push({op:'getQueryParam',url,key});
  for(const [key,value] of [['x','hello world'],['a.b','&+='],['✓','😀']])cases.push({op:'setQueryParam',url,key,value});
 }
}
const run=spawnSync(runner,[],{input:cases.map(v=>JSON.stringify(v)).join('\n')+'\n',encoding:'utf8',maxBuffer:16*1024*1024,timeout:30000});
if(run.status!==0)throw new Error(`Native Ada URL runner failed: ${run.stderr||run.status}`);
const actual=run.stdout.trim().split('\n');
if(actual.length!==cases.length)throw new Error(`URL case count mismatch ${actual.length}/${cases.length}`);
let matched=0;
for(let i=0;i<cases.length;i++){
 const testcase=cases[i];let expected;
 try{
  const value=testcase.op==='isUrl'?isUrl(testcase.url):testcase.op==='getQueryParam'?getQueryParam(testcase.url,testcase.key):setQueryParam(testcase.url,testcase.key,testcase.value);
  expected={ok:true,value};
 }catch{expected={ok:false};}
 const response=JSON.parse(actual[i]);
 if(JSON.stringify(expected)!==JSON.stringify(response)){
  console.error(JSON.stringify({case:testcase,expected,actual:response}));
  process.exitCode=1;
 }else matched++;
}
console.log(`Actual pinned GutTS URL operations vs native Ada: ${matched}/${cases.length} exact results`);
