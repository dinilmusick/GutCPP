#include "gut/minilm_math.hpp"
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <limits>
#include <vector>
#define CHECK(x) do{if(!(x)){std::cerr<<"minilm_math_test:"<<__LINE__<<": "<<#x<<'\n';std::exit(1);}}while(false)
template<class F>bool rejects(F f){try{f();}catch(const std::invalid_argument&){return true;}return false;}
int main(){
 using namespace gut;
 CHECK(minilm_cosine({1,0},{0,1})==0.0);
 CHECK(minilm_cosine({1,1},{1,1})==1.0);
 CHECK(minilm_cosine({1,1},{-1,-1})==-1.0);
 CHECK(minilm_cosine({1,2},{1})==1/std::sqrt(5.0));
 CHECK(std::isnan(minilm_cosine({0,0},{1,0})));
 CHECK(std::isnan(minilm_cosine({1,0},{0,0})));
 CHECK(std::isnan(minilm_cosine({},{})));
 CHECK(minilm_similarity_from_pooled_embeddings({0.5f,0.5f,0.5f,0.5f})==1.0);
 CHECK(rejects([]{(void)minilm_similarity_from_pooled_embeddings({});}));
 CHECK(rejects([]{(void)minilm_similarity_from_pooled_embeddings({1,2,3});}));
 CHECK(std::isnan(minilm_cosine({std::numeric_limits<float>::quiet_NaN()},{1})));
 CHECK(std::abs(minilm_similarity_from_pooled_embeddings({0.2f,0.5f,-0.2f,0.4f})-0.6643638)<1e-4);
 return 0;
}
