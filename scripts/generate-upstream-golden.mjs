// Execute this script with GUTTS_ROOT pointing at a build of the exact pinned GitTS commit.
// It does not reimplement GutTS or fabricate expected outputs.
import {pathToFileURL} from 'node:url';
import {resolve,join} from 'node:path';
import {readFileSync,writeFileSync} from 'node:fs';

const root=resolve(process.env.GUTTS_ROOT || '.upstream');
const sourceSha='3e2cbc0990ba29830ce91c5d1a1d552675d5334c';
const mod=async path=>import(pathToFileURL(join(root,'dist',path)).href);
const arrays=await mod('arrays/array-concepts.js');
const numbers=await mod('numbers/number-concepts.js');
const strings=await mod('strings/string-concepts.js');
const urls=await mod('urls/url-concepts.js');
const safety=await mod('urls/path-safety.js');
const json=await mod('json/json-concepts.js');
const identity=await mod('auth/identityIds.js');
const checks=[
 ['arrays.chunk',()=>arrays.chunk([1,2,3,4,5],2)],
 ['arrays.compact',()=>arrays.compact([1,null,2,null,3])],
 ['arrays.unique',()=>arrays.unique([1,2,1,3])],
 ['arrays.flatten',()=>arrays.flatten([[1,2],[3],4])],
 ['arrays.last',()=>arrays.last([1,2,3])],
 ['numbers.clamp',()=>numbers.clamp(-2,0,12)],
 ['numbers.inRange',()=>numbers.inRange(10,0,10)],
 ['numbers.roundTo',()=>numbers.roundTo(1.2345,2)],
 ['numbers.sum',()=>numbers.sum([1.25,2.5,-3])],
 ['strings.isBlank',()=>strings.isBlank('\u2003 \t')],
 ['strings.startsWithAny',()=>strings.startsWithAny('GutCPP',['Wrong','Gut'])],
 ['strings.endsWithAny',()=>strings.endsWithAny('GutCPP',['JS','CPP'])],
 ['strings.truncateWords',()=>strings.truncateWords('  alpha beta gamma delta  ',2)],
 ['urls.joinUrl',()=>urls.joinUrl('https://example.com///','/a/','//b/','')],
 ['urls.getQueryParam',()=>urls.getQueryParam('https://example.com/path?q=hello+world&q=two','q')],
 ['urls.setQueryParam',()=>urls.setQueryParam('https://example.com/path?a=1','b','hello world')],
 ['urls.isUrl',()=>urls.isUrl('https://example.com/path')],
 ['urls.normalizeStoragePath',()=>safety.normalizeStoragePath('///alpha///beta/file.txt//')],
 ['urls.encodeSafePathSegments',()=>safety.encodeSafePathSegments('alpha/file name.txt')],
 ['urls.resolveTrustedApiUrl',()=>safety.resolveTrustedApiUrl('https://api.example.com','/v1/%2Fproject?x=1').href],
 ['json.parseJson',()=>json.parseJson('{"a":[1,true,null],"b":"hello"}')],
 ['json.stringifyJson',()=>json.stringifyJson({a:[1,true,null]})],
 ['auth.parseIdentityRef',()=>identity.parseIdentityRef({kind:'application',id:'app-1'})],
 ['arrays.chunk.invalid',()=>arrays.chunk([1],0)],
 ['strings.truncateWords.invalid',()=>strings.truncateWords('alpha beta',-1)],
 ['urls.normalizeStoragePath.invalid',()=>safety.normalizeStoragePath('a/%252e%252e/b')],
 ['urls.resolveTrustedApiUrl.invalid',()=>safety.resolveTrustedApiUrl('https://api.example.com','//other.example.com')],
 ['json.parseJson.invalid',()=>json.parseJson('{')],
];
const cases=[];
for(const [id,fn] of checks){
 try{const result=fn();cases.push({id,kind:'value',value:result??null});}
 catch(e){cases.push({id,kind:'error',name:e?.constructor?.name||'Error'});}
}
const fixture={sourceRepository:'dinilmusick/GutTS',sourceSha,cases};
const canonical=JSON.stringify(fixture,null,2)+'\n';
const destination=process.env.GUTCPP_GOLDEN_FILE || 'testdata/upstream-golden/gutts-3e2cbc0.json';
if(process.argv.includes('--check')){
  const committed=readFileSync(destination,'utf8');
  if(committed!==canonical){console.error('Pinned GutTS output diverges from checked-in golden fixture');process.exit(1);}
  console.log('Pinned GutTS golden fixture exact-match: '+cases.length+' cases');
}else{
  console.log('GUTTS_GOLDEN_JSON='+JSON.stringify(fixture));
  if(process.argv.includes('--write'))writeFileSync(destination,canonical);
}
