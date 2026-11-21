// Execute the real immutable GutTS module and native C++ library on the same cases.
// No hand-copied JS implementation or fabricated expected output.
import {readFileSync} from "node:fs";
import {resolve} from "node:path";
import {pathToFileURL} from "node:url";
import {spawnSync} from "node:child_process";
const [upstreamRoot,nativeRunner]=process.argv.slice(2);
if(!upstreamRoot||!nativeRunner) throw Error("usage: verify-storage-path-differential.mjs <compiled-gutts-root> <native-runner>");
const source=await import(pathToFileURL(resolve(upstreamRoot,"dist/urls/path-safety.js")).href);
const fixtures=JSON.parse(readFileSync(new URL("../testdata/upstream-golden/path-safety-cases.json",import.meta.url),"utf8"));
if(fixtures.upstream!=="3e2cbc0990ba29830ce91c5d1a1d552675d5334c")
  throw Error("Wrong pinned GutTS source revision");
let checked=0;
for(const item of fixtures.cases) {
  const fn=item.kind==="normalize"?source.normalizeStoragePath:
           item.kind==="encode"?source.encodeSafePathSegments:null;
  if(!fn)continue; // resolveTrustedApiUrl has a distinct URL adapter/parity obligation.
  let wanted;
  try {
    const result=fn(item.path);
    wanted="ok\t"+Buffer.from(result,"utf8").toString("hex");
  } catch { wanted="error"; }
  const out=spawnSync(resolve(nativeRunner),[item.kind,Buffer.from(item.path,"utf8").toString("hex")],{encoding:"utf8"});
  if(out.error||out.status!==0)
    throw Error(JSON.stringify({operation:item.kind,path:item.path,exit:out.status,stderr:out.stderr,error:String(out.error||"")}));
  const actual=out.stdout.replace(/\r?\n$/,"");
  if(actual!==wanted)
    throw Error(JSON.stringify({operation:item.kind,path:item.path,actual,wanted}));
  checked++;
}
if(checked!==34)throw Error("Fixture inventory changed unexpectedly: "+checked);
console.log("Pinned GutTS vs C++ storage safety exact differential: "+checked+"/"+checked+" cases passed");
