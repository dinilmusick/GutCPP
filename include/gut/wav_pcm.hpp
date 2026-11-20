#pragma once
// Bounded RIFF/WAVE PCM and IEEE-float input for native Whistle transcription.
// Pure portable C++17: no file IO, global state, dynamic codecs or audio SDK.
#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <limits>
#include <stdexcept>
#include <string>
#include <vector>

namespace gut {
namespace wav_detail {
inline std::uint16_t u16(const std::vector<std::uint8_t>& b,std::size_t at) {
  if(at>b.size()||b.size()-at<2)throw std::invalid_argument("short WAVE field");
  return static_cast<std::uint16_t>(b[at]|(std::uint16_t(b[at+1])<<8));
}
inline std::uint32_t u32(const std::vector<std::uint8_t>& b,std::size_t at) {
  if(at>b.size()||b.size()-at<4)throw std::invalid_argument("short WAVE field");
  return std::uint32_t(b[at])|(std::uint32_t(b[at+1])<<8)|
    (std::uint32_t(b[at+2])<<16)|(std::uint32_t(b[at+3])<<24);
}
inline std::uint64_t u64(const std::vector<std::uint8_t>& b,std::size_t at) {
  return std::uint64_t(u32(b,at))|(std::uint64_t(u32(b,at+4))<<32);
}
inline bool tag(const std::vector<std::uint8_t>& b,std::size_t at,const char* id) {
  return at<=b.size()&&b.size()-at>=4&&
    b[at]==static_cast<std::uint8_t>(id[0])&&
    b[at+1]==static_cast<std::uint8_t>(id[1])&&
    b[at+2]==static_cast<std::uint8_t>(id[2])&&
    b[at+3]==static_cast<std::uint8_t>(id[3]);
}
inline float sample(const std::vector<std::uint8_t>& b,std::size_t at,unsigned format,unsigned bits){
  double x=0;
  if(format==1) {
    if(bits==8)x=(static_cast<int>(b[at])-128)/128.0;
    else if(bits==16)x=static_cast<std::int16_t>(u16(b,at))/32768.0;
    else if(bits==24){
      std::int32_t v=std::int32_t(b[at])|(std::int32_t(b[at+1])<<8)|(std::int32_t(b[at+2])<<16);
      if(v&0x800000)v-=0x1000000;
      x=v/8388608.0;
    }else x=static_cast<std::int32_t>(u32(b,at))/2147483648.0;
  }else if(bits==32){
    const auto packed=u32(b,at);
    float v=0;static_assert(sizeof(v)==sizeof(packed),"IEEE float32 required");
    std::memcpy(&v,&packed,sizeof(v));x=v;
  }else{
    const auto packed=u64(b,at);
    double v=0;static_assert(sizeof(v)==sizeof(packed),"IEEE float64 required");
    std::memcpy(&v,&packed,sizeof(v));x=v;
  }
  if(!std::isfinite(x)||x < -1.0||x > 1.0)throw std::invalid_argument("WAVE audio contains invalid samples");
  return static_cast<float>(x);
}
} // namespace wav_detail

// Return exactly 16 kHz mono float32 in [-1,1]; accepts RIFF/WAVE integer
// PCM 8/16/24/32 or IEEE_FLOAT 32/64, channels 1..8, rate 8000..96000 Hz.
// Higher quality resampling and compressed formats require a separate codec.
inline std::vector<float> decode_wav_to_16khz(const std::vector<std::uint8_t>& bytes){
  using namespace wav_detail;
  if(bytes.size()<44||!tag(bytes,0,"RIFF")||!tag(bytes,8,"WAVE"))
    throw std::invalid_argument("RIFF/WAVE header required");
  const auto riff_size=u32(bytes,4);
  if(riff_size<36||std::uint64_t(riff_size)+8>bytes.size())
    throw std::invalid_argument("invalid RIFF byte length");
  const std::size_t end=std::size_t(riff_size)+8;
  unsigned format=0,channels=0,bits=0,rate=0;
  std::size_t data_at=0,data_bytes=0;
  bool have_fmt=false,have_data=false;
  for(std::size_t pos=12;pos<end;){
    if(end-pos<8)throw std::invalid_argument("truncated WAVE chunk header");
    const std::size_t size=u32(bytes,pos+4);
    const std::size_t start=pos+8;
    if(size>end-start||((size&1u)!=0&&size==end-start))
      throw std::invalid_argument("truncated WAVE chunk payload or pad");
    if(tag(bytes,pos,"fmt ")){
      if(have_fmt||size<16)throw std::invalid_argument("invalid WAVE fmt chunk");
      have_fmt=true;
      format=u16(bytes,start);channels=u16(bytes,start+2);rate=u32(bytes,start+4);
      const auto byte_rate=u32(bytes,start+8);
      const auto block_align=u16(bytes,start+12);
      bits=u16(bytes,start+14);
      if(format==0xFFFE){
        // WAVE_FORMAT_EXTENSIBLE: GUID's first two bytes identify PCM/float.
        if(size<40||u16(bytes,start+16)<22||(u16(bytes,start+18)!=0&&u16(bytes,start+18)!=bits))
          throw std::invalid_argument("invalid extensible WAVE format");
        format=u16(bytes,start+24);
        static constexpr std::uint8_t guid_tail[]={0,0,0,0,0x10,0,0x80,0,0,0xAA,0,0x38,0x9B,0x71};
        // Validate canonical KSDATAFORMAT_SUBTYPE_PCM/IEEE_FLOAT GUID.
        for(std::size_t i=0;i<14;++i)
          if(bytes[start+26+i]!=guid_tail[i])throw std::invalid_argument("unsupported WAVE subformat GUID");
      }
      if((format!=1&&format!=3)||channels<1||channels>8||rate<8000||rate>96000||
         (format==1&&bits!=8&&bits!=16&&bits!=24&&bits!=32)||
         (format==3&&bits!=32&&bits!=64)||
         block_align!=channels*(bits/8)||byte_rate!=rate*block_align)
        throw std::invalid_argument("unsupported WAVE audio encoding");
    }else if(tag(bytes,pos,"data")){
      if(have_data)throw std::invalid_argument("multiple WAVE data chunks unsupported");
      have_data=true;data_at=start;data_bytes=size;
    }
    pos=start+size+(size&1u);
  }
  if(!have_fmt||!have_data||!data_bytes)throw std::invalid_argument("WAVE missing fmt or audio data");
  const auto bytes_per_frame=std::size_t(channels)*(bits/8);
  if(data_bytes%bytes_per_frame)throw std::invalid_argument("partial WAVE audio frame");
  const std::size_t frames=data_bytes/bytes_per_frame;
  if(!frames||frames>std::size_t(rate)*600u)throw std::invalid_argument("WAVE duration outside 600 seconds");
  const std::uint64_t samples64=(std::uint64_t(frames)*16000u+rate/2)/rate;
  if(samples64==0||samples64>16000u*600u)throw std::invalid_argument("invalid WAVE resampled length");
  std::vector<float> mono;mono.reserve(frames);
  for(std::size_t i=0;i<frames;++i){
    double sum=0.0;
    for(std::size_t c=0;c<channels;++c)
      sum+=sample(bytes,data_at+i*bytes_per_frame+c*(bits/8),format,bits);
    mono.push_back(static_cast<float>(sum/channels));
  }
  if(rate==16000)return mono;
  std::vector<float> out(static_cast<std::size_t>(samples64));
  for(std::size_t i=0;i<out.size();++i){
    const std::uint64_t source=std::uint64_t(i)*rate;
    const std::size_t begin=std::min<std::size_t>(source/16000u,mono.size()-1);
    const std::size_t next=std::min(begin+1,mono.size()-1);
    const float alpha=static_cast<float>(source%16000u)/16000.0f;
    out[i]=mono[begin]+(mono[next]-mono[begin])*alpha;
  }
  return out;
}
} // namespace gut
