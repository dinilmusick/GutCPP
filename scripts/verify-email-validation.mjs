// Compare native C++17 default is_email against immutable GutTS validator.js wrapper.
// No hand-written expected results: the exact pinned package executes each case.
import {spawnSync} from 'node:child_process';
import {resolve,join} from 'node:path';
import {pathToFileURL} from 'node:url';
if(process.argv.length!==4)throw Error('Usage: verify-email-validation.mjs <pinned GutTS root> <C++ runner>');
const modulePath=pathToFileURL(join(resolve(process.argv[2]),'dist/strings/string-concepts.js')).href;
const {isEmail}=await import(modulePath);
const cases=[];
function add(email){cases.push(email);}
const locals=[
 'alice','alice+tag','user.name','a_b','A1!#$%&\'*+-/=?^_`{|}~',
 'user..name','user.','.user','','.','""','"hello world"','"a@b"',
 '"say\\"hello"','"has\\\\slash"','"a\tb"','"has\nnewline"',
 '"unterminated','unterminated"','"evil\\q"','"\u0000"','"\u001b"',
 'café','é','éé','Straße','Αθήνα','漢字','á.b','é+tag','中','😀',
 '\u00a0','\u00a1','\u009f','\ufeff','\uffff','\uff01','\ud83d\ude00',
 '"🙂"','"a\\@b"','`backtick','name<foo','x'.repeat(63),'x'.repeat(64),
 'x'.repeat(65),'é'.repeat(30),'é'.repeat(33),'a@b'
];
const domains=[
 'example.com','sub.example.org','example.co.uk','e.xn--b','e.xn--',
 'e.xn123','e.xn--punycode','a.中国','a.éé','café.example',
 'e.öö','Éxample.COM','a.১২','a.٣٣','a.中','a.x', 'a.123','a.a1','a.1a',
 'localhost','', 'a.', '.com','a..com','a.-example','-example.com','example-.com',
 'under_score.com','a.🎶','a.😀','a.\uff21\uff22','a.\u00a9','a.\u00aa',
 'a.\u00ab','a.\u200e\u200f','a.\u00a0','a.\ufeff','a.\u0000',
 'x'.repeat(63)+'.com','x'.repeat(64)+'.com',
 'x.'+'x'.repeat(63)+'.com','x.'+'x'.repeat(64)+'.com',
 'e.'+'a'.repeat(240)+'.com','e.'+'a'.repeat(240)+'.org',
 'a.'+'ø'.repeat(64), 'a.'+'ø'.repeat(62),
 'test.example.com.', 'test..com','a.xn--x-','a.xn--1234','a.xn-'
];
for(const local of locals)for(const domain of domains)add(local+'@'+domain);
for(const str of ['','no-at-sign','@@@','foo@@example.com','foo@bar@baz.com',
 'a..b@example.com','x <a@example.com>','foo@example.com\r\n',
 '"x"@example.com','"@"@example.com','"a@b"@example.com',
 'a@sub..example.com','x@a.com.','a@xn--.com','a@xn--a.com',
 'a@under_score.com','x@FOO.BAR','x@localhost',
 'x@example.\ufffe','x@example.\ufeff',
 'a'.repeat(254)+'@x.io', 'a'.repeat(64)+'@'+'a'.repeat(63)+'.org',
 'a'.repeat(63)+'@'+'a'.repeat(63)+'.org',
 '"foo\\\" bar"@sub.example.com'])add(str);
let state=0xbabecafe;
function rand(){state^=state<<13;state^=state>>>17;state^=state<<5;return state>>>0;}
const localChars=['a','z','7','-','.','_','+','!',' ', '"','\\',
 'é','À','中','🙂','\u00a0','\u00a1','\u009f','\uff0e'];
const domainChars=['a','x','7','-','.','_','é','中','\u00a0','\uff1f','🚀'];
for(let i=0;i<2500;i++){
 let local='',domain='';
 for(let j=0,n=rand()%80;j<n;j++)local+=localChars[rand()%localChars.length];
 for(let j=0,n=rand()%90;j<n;j++)domain+=domainChars[rand()%domainChars.length];
 add(local+'@'+domain);
}
const stdin=cases.map(JSON.stringify).join('\n')+'\n';
const native=spawnSync(resolve(process.argv[3]),[],{input:stdin,encoding:'utf8',timeout:30000,maxBuffer:20*1024*1024});
if(native.error||native.status!==0)throw Error(`Native runner failed: ${native.stderr||native.error}`);
const lines=native.stdout.trim().split(/\r?\n/);
if(lines.length!==cases.length)throw Error(`Native case count ${lines.length}/${cases.length}`);
let differences=0;
for(let i=0;i<cases.length;i++){
 const expected=isEmail(cases[i]);
 const actual=lines[i]==='true';
 if(actual!==expected){differences++;if(differences<=30)console.error(JSON.stringify({case:cases[i],expected,actual}));}
}
console.log(`Pinned validator.js isEmail vs C++17 differential ${cases.length-differences}/${cases.length}; mismatches ${differences}`);
if(differences)process.exit(1);
