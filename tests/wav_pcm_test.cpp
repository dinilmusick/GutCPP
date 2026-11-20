#include "gut/wav_pcm.hpp"
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <limits>
#include <vector>
#include <cstdint>
#define CHECK(x) do{if(!(x)){std::cerr<<"WAV test "<<__LINE__<<" failed: "<<#x<<'\n';return 1;}}while(false)
namespace {
void put16(std::vector<std::uint8_t>& b,unsigned v){b.push_back(v&255);b.push_back((v>>8)&255);}
void put32(std::vector<std::uint8_t>& b,std::uint32_t v){put16(b,v&65535);put16(b,v>>16);}
std::vector<std::uint8_t> wav(unsigned encoding,unsigned bits,unsigned channels,unsigned rate,unsigned frames){
  std::vector<std::uint8_t> out;
  auto tag=[&](const char* s){for(int i=0;i<4;++i)out.push_back(static_cast<std::uint8_t>(s[i]));};
  unsigned bytes=bits/8, data_len=frames*channels*bytes;
  tag("RIFF");put32(out,36+data_len+(data_len&1));tag("WAVE");
  tag("fmt ");put32(out,16);put16(out,encoding);put16(out,channels);put32(out,rate);
  put32(out,rate*channels*bytes);put16(out,channels*bytes);put16(out,bits);
  tag("data");put32(out,data_len);
  for(unsigned i=0;i<frames;++i)for(unsigned c=0;c<channels;++c){
    // Source frames alternate exact silence and 50% amplitude.
    const double value=(i%2==0?0.0:0.5)*(c==0?1.0:-1.0);
    if(encoding==1){
      if(bits==8)out.push_back(static_cast<std::uint8_t>(128+value*128));
      else if(bits==16)put16(out,static_cast<std::uint16_t>(static_cast<std::int16_t>(value*32768)));
      else if(bits==24){auto x=static_cast<std::int32_t>(value*8388608);for(int j=0;j<3;++j)out.push_back((x>>(j*8))&255);}
      else put32(out,static_cast<std::uint32_t>(static_cast<std::int32_t>(value*2147483648.0)));
    }else if(bits==32){float v=static_cast<float>(value);std::uint32_t x;std::memcpy(&x,&v,4);put32(out,x);}
    else {double v=value;std::uint64_t x;std::memcpy(&x,&v,8);put32(out,static_cast<std::uint32_t>(x));put32(out,static_cast<std::uint32_t>(x>>32));}
  }
  if(data_len&1)out.push_back(0);
  return out;
}
}
int main(){
  for(auto fmt:{1u,3u})for(auto bits:{8u,16u,24u,32u,64u}){
    if((fmt==1&&bits==64)||(fmt==3&&bits!=32&&bits!=64))continue;
    for(auto channels:{1u,2u}){
      auto bytes=wav(fmt,bits,channels,16000,100);
      auto decoded=gut::decode_wav_to_16khz(bytes);
      CHECK(decoded.size()==100);
      CHECK(decoded[0]==0.0f);
      CHECK(std::fabs(decoded[1]-(channels==1?.5f:0.f))<.0001f);
      // Sample rates other than 16 kHz must be resampled deterministically.
      bytes=wav(fmt,bits,channels,8000,100);
      CHECK(gut::decode_wav_to_16khz(bytes).size()==200);
      bytes=wav(fmt,bits,channels,48000,300);
      CHECK(gut::decode_wav_to_16khz(bytes).size()==100);
    }
  }
  auto raw=wav(1,16,1,16000,100);
  auto rejects=[&](std::vector<std::uint8_t> bytes){try{(void)gut::decode_wav_to_16khz(bytes);return false;}catch(const std::invalid_argument&){return true;}};
  CHECK(rejects({}));CHECK(rejects({1,2,3}));
  auto bad=raw;bad[0]='N';CHECK(rejects(bad));
  bad=raw;bad[4]=255;bad[5]=255;bad[6]=255;bad[7]=127;CHECK(rejects(bad));
  bad=raw;bad.resize(60);CHECK(rejects(bad));
  bad=raw;bad[20]=7;CHECK(rejects(bad));
  bad=raw;bad[22]=0;CHECK(rejects(bad));
  bad=raw;bad[24]=0;bad[25]=0;bad[26]=0;bad[27]=0;CHECK(rejects(bad));
  bad=raw;bad[32]=1;CHECK(rejects(bad));
  bad=raw;bad[40]=201;bad[41]=0;CHECK(rejects(bad));
  // Malformed integer sample with a short final frame.
  bad=raw;bad[40]=199;bad[41]=0;bad[4]=static_cast<std::uint8_t>(36+200);CHECK(rejects(bad));
  // Canonical extensible PCM format with explicit channel mask and subtype GUID.
  auto ext=raw;
  const std::vector<std::uint8_t> extension={22,0,16,0,0,0,0,0,
      1,0,0,0,0,0,0x10,0,0x80,0,0,0xAA,0,0x38,0x9B,0x71};
  ext.insert(ext.begin()+36,extension.begin(),extension.end());
  ext[4]=static_cast<std::uint8_t>((36+200+24)&255);ext[5]=static_cast<std::uint8_t>((36+200+24)>>8);
  ext[16]=40;ext[20]=0xFE;ext[21]=0xFF;
  CHECK(gut::decode_wav_to_16khz(ext).size()==100);
  ext[46]=0x01;CHECK(rejects(ext)); // Reject corrupt subtype GUID.
  ext=raw; // Unknown JUNK chunk with mandatory even padding.
  const std::vector<std::uint8_t> junk={'J','U','N','K',3,0,0,0,'x','y','z',0};
  ext.insert(ext.begin()+36,junk.begin(),junk.end());
  ext[4]=static_cast<std::uint8_t>(36+200+junk.size());
  CHECK(gut::decode_wav_to_16khz(ext).size()==100);
  auto invalid_float=wav(3,32,1,16000,2);
  invalid_float[44]=0;invalid_float[45]=0;invalid_float[46]=0xC0;invalid_float[47]=0x7F;
  CHECK(rejects(invalid_float)); // NaN sample
  invalid_float=wav(3,32,1,16000,2);
  invalid_float[44]=0;invalid_float[45]=0;invalid_float[46]=0xC0;invalid_float[47]=0x3F;
  CHECK(rejects(invalid_float)); // 1.5 amplitude
  // The parser must not read past the input even for randomly corrupted fields.
  std::uint32_t state=0xBB001337u;
  for(int i=0;i<800;++i){
    state^=state<<13;state^=state>>17;state^=state<<5;
    auto candidate=raw;
    const std::size_t offset=state%candidate.size();
    candidate[offset]^=static_cast<std::uint8_t>((state>>16)|1u);
    try{
      const auto samples=gut::decode_wav_to_16khz(candidate);
      CHECK(samples.size()<=16000u*600u);
      for(float s:samples)CHECK(std::isfinite(s)&&s>=-1.0f&&s<=1.0f);
    }catch(const std::invalid_argument&){}
  }
  // Excess bytes are permitted only after the declared RIFF payload.
  bad=raw;bad.insert(bad.end(),{99,98,97});CHECK(gut::decode_wav_to_16khz(bad).size()==100);
}
