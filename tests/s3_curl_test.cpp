#include "gut/curl_http.hpp"
#include "gut/s3.hpp"
#include <cstdlib>
#include <iostream>
#include <string>
#define CHECK(x) do{if(!(x)){std::cerr<<"s3_curl_test:"<<__LINE__<<": "<<#x<<'\n';std::exit(1);}}while(false)
int main(int argc,char** argv){
 if(argc!=2)return 2;
 using namespace gut;
 const std::string origin="http://127.0.0.1:"+std::string(argv[1]);
 curl_http_adapter curl;
 luth_http_request set_cookie;set_cookie.url=origin+"/set-cookie";
 CHECK(curl(set_cookie).status==200);
 const auto transport=[&](const luth_http_request& request){return curl(request);};
 s3_options config;config.endpoint_origin=origin;config.allow_loopback_test_origin=true;
 s3_client client(transport,[]{return s3_credentials{"AKIDEXAMPLE","secret-example","token_example"};},config,[]{return "20261120T141900Z";});
 const std::string bytes("\0native\xff",8);
 CHECK(client.get_object("test-bucket","folder/hello world.txt")==bytes);
 CHECK(client.head_object("test-bucket","folder/hello world.txt").at("etag")=="\"testetag\"");
 CHECK(client.put_object("test-bucket","folder/hello world.txt",bytes).at("etag")=="\"testetag\"");
 CHECK(client.list_objects_v2("test-bucket","prefix",3).find("<ListBucketResult>")!=std::string::npos);
 CHECK(client.copy_object("test-bucket","copy","test-bucket","folder/hello world.txt").find("CopyObjectResult")!=std::string::npos);
 client.delete_object("test-bucket","copy");
 CHECK(client.delete_objects("test-bucket",{"first","second"}).find("DeleteResult")!=std::string::npos);
 luth_http_request check_cookie;check_cookie.url=origin+"/check-cookie";
 CHECK(curl(check_cookie).status==200);
 return 0;
}
