// True source-executed immutable GutTS Transformers.js AutoTokenizer paired
// tokenization against C++ libembedding tokens (no mocked or copied outputs).
import {createRequire} from "node:module";
import {execFileSync} from "node:child_process";
import {resolve} from "node:path";
import {pathToFileURL} from "node:url";
const root=resolve(process.argv[2]??".upstream");
const runner=resolve(process.argv[3]??"./build-ettin/gutcpp_ettin_tokenizer_runner");
const tokenizerFile=resolve(process.argv[4]??".ettin-model/tokenizer.json");
const requireFromSource=createRequire(resolve(root,"package.json"));
const transformersModule=await import(pathToFileURL(requireFromSource.resolve("@huggingface/transformers")).href);
const AutoTokenizer=transformersModule.AutoTokenizer??transformersModule.default?.AutoTokenizer;
if(!AutoTokenizer)throw Error("The original pinned GutTS Transformers.js package lacks AutoTokenizer");
const pinned="3253335c59c0d70ababfdee87ad5390b4d1eff4d";
const tokenizer=await AutoTokenizer.from_pretrained("cross-encoder/ettin-reranker-17m-v1",{revision:pinned});
const inputs=[
 ["What is the Red Planet?","Mars is known as the Red Planet."],
 ["What is the Red Planet?","Venus is Earth's closest planetary neighbor."],
 ["How does a compiler work?","A compiler converts source programs to machine instructions."],
 ["Where is Pune?","Pune is a city in Maharashtra, India."],
 ["What's the capital of France?","Paris is France's capital."],
 ["Should native programs validate Unicode?","Yes, reject invalid UTF-8 input."],
 ["A short query","A short document"],
 ["capital?","シンガポール東京"],
 ["mañana café","français déjà vu"],
 ["हिंदी प्रश्न","शिक्षा और संस्कृति"],
 ["മലയാളം","മലയാളം ഭാഷ"],
 ["مرحبا بالعالم","لغة عربية"],
 ["What does this mean?","👨‍👩‍👦🇮🇳👍🏽"],
 ["Unicode e\u0301","école e\u0301co"],
 ["Hello","你好，世界"],
 ["a b c","a b c d e f"],
];
// Longest-first truncation cases: single and paired token counts exceeding
// 512, both balanced and imbalanced, with Unicode and punctuation.
// These execute the real Transformers.js tokenizer; no expected IDs copied.
const longTexts=[
  "Mars is known as the Red Planet. ".repeat(95),
  "What is the capital of France? ".repeat(75),
  "हिंदी प्रश्न और शिक्षा ".repeat(130),
  "The document discusses a real transformer model. ".repeat(76),
  "🇮🇳👨‍👩‍👦 Unicode model tokens ".repeat(90),
];
for(let i=0;i<longTexts.length;++i){
  inputs.push([longTexts[i],longTexts[(i+1)%longTexts.length]]);
  inputs.push(["Short query",longTexts[i]]);
  inputs.push([longTexts[i],"Short document"]);
}
const data=t=>{
 if(!t||typeof t!=="object"||!t.data)throw Error("Missing genuine source token tensor");
 return Array.from(t.data,x=>Number(x));
};
let matches=0;
for(const [query,document] of inputs) {
  const upstream=await tokenizer([query],{text_pair:[document],padding:true,truncation:true});
  const expected={
    input_ids:data(upstream.input_ids),
    attention_mask:data(upstream.attention_mask),
    token_type_ids:upstream.token_type_ids?data(upstream.token_type_ids):null,
  };
  const raw=execFileSync(runner,[tokenizerFile,query,document],{timeout:12000,maxBuffer:1<<19});
  const native=JSON.parse(raw.toString("utf8"));
  for(const key of ["input_ids","attention_mask","token_type_ids"]) {
    if(key==="token_type_ids"&&!expected[key])continue;
    if(JSON.stringify(native[key])!==JSON.stringify(expected[key]))
      throw Error(JSON.stringify({error:"Pinned GutTS↔native Ettin paired token divergence",
        model_revision:pinned,query,document,key,
        original:expected[key],native:native[key]}));
  }
  matches++;
}
console.log("GutCPP original pinned Transformers.js paired Ettin tokenizer: "+matches+"/"+inputs.length+" exact native token/mask comparisons passed");
