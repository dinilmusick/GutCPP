// Executable pinned GutTS Lodash wrapper vs real ICU-backed C++17.
import {spawnSync} from 'node:child_process';
import {resolve,join} from 'node:path';
import {pathToFileURL} from 'node:url';
if(process.argv.length!==4)throw new Error('usage: verify-icu-strings.mjs <GutTS-root> <native-runner>');
const reference=await import(pathToFileURL(join(resolve(process.argv[2]),'dist/strings/string-concepts.js')).href);
const values=[
 'FRED','foo BAR','Foo Bar','HTMLParser Value','FooBar BAZ','hello---WORLD',
 'caféCrème','café Crème','ÉCOLE','Straße','Mañana señor','naïve Café','São Paulo',
 'Æther','fooBARBaz','FOOBar','a1B2','🤖 HUMAN','🤖hello',
 'ΣΜΑ','İSTANBUL','Αθήνα','Москва ПРИВЕТ','ไทย ภาษาไทย',
 '中文 测试','smile 🙂 World','👨‍👩‍👧‍👦 family','hello 👩‍💻 WORLD',
 'e\u0301cole','🇮🇳India 🇬🇧UK','','...','🚀🚀🚀'
];
const cases=[];
for(const value of values)
 for(const op of ['capitalize','camelCase','kebabCase','snakeCase'])cases.push({op,value});
for(const value of ['abcdefgh','abc','café😀hello','🇮🇳abc','e\u0301cole','👩‍💻hello','🙂🙂🙂x','😀ab'])
 for(const maxLength of [0,1,2,3,4,5,8])cases.push({op:'truncate',value,maxLength,suffix:'…'});
for(const value of ['hello','hello world','abcd😀xyz'])
 for(const suffix of ['', '.', '...', 'long'])cases.push({op:'truncate',value,maxLength:4,suffix});
function runReference({op,value,maxLength,suffix}) {
 try {
   return {ok:true,value:op==='truncate'?reference.truncate(value,maxLength,suffix):reference[op](value)};
 }catch{return {ok:false};}
}
const native=spawnSync(resolve(process.argv[3]),[],{
 input:cases.map(x=>JSON.stringify(x)).join('\n')+'\n',
 encoding:'utf8',timeout:30000,maxBuffer:20*1024*1024
});
if(native.error||native.status!==0)throw Error('native ICU runner failed: '+(native.stderr||native.error));
const lines=native.stdout.trim().split(/\r?\n/);
if(lines.length!==cases.length)throw Error('native response count '+lines.length+'/'+cases.length);
let differences=0;
for(let i=0;i<cases.length;i++) {
 const expected=runReference(cases[i]),actual=JSON.parse(lines[i]);
 if(JSON.stringify(expected)!==JSON.stringify(actual)) {
   differences++;
   if(differences<=25)console.error(JSON.stringify({case:cases[i],expected,actual}));
 }
}
console.log('Pinned GutTS vs ICU Unicode differential '+(cases.length-differences)+'/'+cases.length+' matched; divergences '+differences);
if(differences)process.exit(1);
