// Independent source-surface audit: this walks the ACTUAL pinned TypeScript barrels.
// A JSON matrix by itself is not proof that all public exports were enumerated.
import {readFileSync,existsSync} from "node:fs";
import {createRequire} from "node:module";
import {dirname,resolve,join} from "node:path";
const root=resolve(process.argv[2]??".upstream");
const matrix=JSON.parse(readFileSync(new URL("../docs/wave1-contract-matrix.json",import.meta.url),"utf8"));
const reference="3e2cbc0990ba29830ce91c5d1a1d552675d5334c";
if(matrix.reference?.sha!==reference||matrix.contracts?.length!==195||matrix.row_count!==195)
  throw Error("Pinned matrix SHA or row count is invalid");
if(!existsSync(join(root,"package.json")))throw Error("Immutable TypeScript source checkout missing");
const sourcePkg=JSON.parse(readFileSync(join(root,"package.json"),"utf8"));
if(sourcePkg.version!=="2.2.3")throw Error("Unexpected pinned GutTS version");
const require=createRequire(join(root,"package.json"));
const ts=require("typescript");
const symbolNames=new Set();
const visited=new Map();
function hasExport(node) {
  return (ts.getModifiers(node)??[]).some(m=>m.kind===ts.SyntaxKind.ExportKeyword);
}
function sourcePath(from,spec) {
  const file=resolve(dirname(from),spec.replace(/\.js$/,".ts"));
  for(const candidate of [file,file+".ts",join(file,"index.ts")])
    if(existsSync(candidate))return candidate;
  throw Error("Cannot resolve pinned TypeScript reexport: "+from+" -> "+spec);
}
function collectNames(file) {
  const full=resolve(file);
  if(visited.has(full))return visited.get(full);
  const symbols=new Set();
  visited.set(full,symbols); // cyclic barrels use the same in-progress set
  const source=ts.createSourceFile(full,readFileSync(full,"utf8"),ts.ScriptTarget.Latest,true,ts.ScriptKind.TS);
  function binding(name) {
    if(ts.isIdentifier(name))symbols.add(name.text);
    else if(ts.isObjectBindingPattern(name)||ts.isArrayBindingPattern(name))
      for(const e of name.elements)if(ts.isBindingElement(e))binding(e.name);
  }
  for(const node of source.statements) {
    if(ts.isExportDeclaration(node)) {
      if(node.exportClause) {
        if(ts.isNamedExports(node.exportClause))
          for(const e of node.exportClause.elements)symbols.add(e.name.text);
        else if(ts.isNamespaceExport(node.exportClause))symbols.add(node.exportClause.name.text);
      } else if(node.moduleSpecifier&&ts.isStringLiteral(node.moduleSpecifier)) {
        const spec=node.moduleSpecifier.text;
        if(!spec.startsWith("."))throw Error("Unexpanded external star export: "+spec);
        for(const s of collectNames(sourcePath(full,spec)))symbols.add(s);
      }
      continue;
    }
    if(!hasExport(node))continue;
    if(ts.isVariableStatement(node))for(const d of node.declarationList.declarations)binding(d.name);
    else if((ts.isFunctionDeclaration(node)||ts.isClassDeclaration(node)||
             ts.isInterfaceDeclaration(node)||ts.isTypeAliasDeclaration(node)||
             ts.isEnumDeclaration(node)||ts.isModuleDeclaration(node))&&node.name)
      symbols.add(node.name.text);
    else if(ts.isExportAssignment(node))symbols.add("default");
  }
  return symbols;
}
for(const [entrypoint,record] of Object.entries(sourcePkg.exports)) {
  if(entrypoint.endsWith(".json"))continue;
  const compiled=typeof record==="string"?record:record.import??record.default;
  if(typeof compiled!=="string"||!compiled.startsWith("./dist/")||!compiled.endsWith(".js"))
    throw Error("Unrecognized published entrypoint "+entrypoint);
  const source=join(root,compiled.replace(/^\.\/dist\//,"src/").replace(/\.js$/,".ts"));
  for(const symbol of collectNames(source))symbolNames.add(symbol);
}
const synthetic=new Set(["rootExports","browserExports","workerExports","serverExports"]);
const listed=new Set();
for(const c of matrix.contracts) {
  if(typeof c.upstream_symbol!=="string"||!c.upstream_symbol.trim())throw Error("Empty contract name");
  if(listed.has(c.upstream_symbol))throw Error("Duplicate contract "+c.upstream_symbol);
  listed.add(c.upstream_symbol);
  if(!["entrypoint_obligation","public_symbol"].includes(c.kind))throw Error("Unknown contract kind "+c.upstream_symbol);
  if(c.kind==="entrypoint_obligation"!==synthetic.has(c.upstream_symbol))
    throw Error("Synthetic entrypoint misclassified "+c.upstream_symbol);
  if(c.upstream_source&&!existsSync(join(root,c.upstream_source)))
    throw Error("Source row path missing in pinned upstream: "+c.upstream_symbol+" "+c.upstream_source);
  if(c.native_source&&!existsSync(resolve(c.native_source)))
    throw Error("Native candidate source missing: "+c.upstream_symbol+" "+c.native_source);
  if(c.executable_test&&!existsSync(resolve(c.executable_test)))
    throw Error("Referenced executable test source missing: "+c.upstream_symbol+" "+c.executable_test);
  if(!["fully_implemented_and_behaviorally_verified","implemented_needs_verification","partially_implemented","platform_exception","unimplemented"].includes(c.classification))
    throw Error("Unknown parity classification for "+c.upstream_symbol);
  if(c.classification==="fully_implemented_and_behaviorally_verified"&&
    (!c.native_symbol||!c.native_source||!c.executable_test||!c.evidence?.length))
    throw Error("Fully verified contract missing source/test evidence "+c.upstream_symbol);
  if(c.classification==="unimplemented"&&c.native_symbol)
    throw Error("Unimplemented contract incorrectly lists native execution "+c.upstream_symbol);
}
const omitted=[...symbolNames].filter(name=>!listed.has(name)).sort();
const unexported=[...listed].filter(name=>!symbolNames.has(name)&&!synthetic.has(name)).sort();
if(omitted.length||unexported.length)
  throw Error(JSON.stringify({problem:"Published TypeScript export inventory mismatch",source_count:symbolNames.size,
                             matrix_symbol_count:matrix.contracts.length-synthetic.size,omitted,unexported}));
const classifications={};
for(const c of matrix.contracts)classifications[c.classification]=(classifications[c.classification]||0)+1;
for(const [key,value] of Object.entries(classifications))
  if(matrix.summary?.[key]!==value)throw Error("Classification count drift: "+key);
console.log("GutCPP pinned TypeScript exported surface: "+symbolNames.size+" exact published names, plus "+synthetic.size+" entrypoint obligations; 195 distinct matrix rows matched.");
console.log("Conservative parity categories: "+JSON.stringify(classifications));
