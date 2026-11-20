#include "gut/curl_http.hpp"
#include "gut/google_drive.hpp"
#include <cstdlib>
#include <iostream>
#include <string>
#define CHECK(x) do{if(!(x)){std::cerr<<"google_drive_curl_test:"<<__LINE__<<": "<<#x<<'\n';std::exit(1);}}while(false)
int main(int argc,char** argv){
 if(argc!=2) return 2;
 using namespace gut;
 const std::string origin="http://127.0.0.1:"+std::string(argv[1]);
 curl_http_adapter curl;
 luth_http_transport transport=[&](const luth_http_request& req){return curl(req);};
 // Establish a real libcurl cookie jar that must be retained for BFF sessions.
 luth_http_request set_cookie;set_cookie.url=origin+"/set-cookie";
 CHECK(curl(set_cookie).status==200);
 google_drive_client drive(transport,[]{return "integration_token";},{origin,true,10000,1024*1024});
 const auto list=drive.list_files("name contains 'Test'",{},2);
 CHECK(list.at("files").as<json_value::array>().size()==1);
 const auto meta=drive.get_metadata("file_1");
 CHECK(meta.at("id").as<std::string>()=="file_1");
 const std::string bytes("\0native\xff",8);
 CHECK(drive.download_media("file_1")==bytes);
 CHECK(drive.create_metadata("New Document","text/plain").at("id").as<std::string>()=="created");
 CHECK(drive.upload_media(bytes,"application/octet-stream").at("id").as<std::string>()=="uploaded");
 CHECK(drive.update_media("file_1",bytes,"application/octet-stream").at("id").as<std::string>()=="updated");
 CHECK(drive.rename_file("file_1","Renamed").at("id").as<std::string>()=="renamed");
 CHECK(drive.copy_file("file_1","Copied").at("id").as<std::string>()=="copied");
 CHECK(drive.export_file("file_1","text/plain")=="exported");
 drive.delete_file("file_1");
 luth_http_request check_cookie;check_cookie.url=origin+"/check-cookie";
 CHECK(curl(check_cookie).status==200);
 return 0;
}
