#include "gut/google_drive.hpp"
#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>
#define CHECK(x) do{if(!(x)){std::cerr<<"google_drive_test:"<<__LINE__<<": "<<#x<<'\n';std::exit(1);}}while(false)
namespace {
template<typename F>bool throws(F fn){try{fn();return false;}catch(const std::exception&){return true;}}
}
int main(){
 using namespace gut;
 std::vector<luth_http_request> requests;
 std::string token="test_token";
 bool deny=false;
 luth_http_transport transport=[&](const luth_http_request& req){
   requests.push_back(req);
   if(deny)return luth_http_response{403,R"({"error":{"message":"secret"}})",{}};
   if(req.method=="GET" && req.url.find("alt=media")!=std::string::npos)
     return luth_http_response{200,std::string("a\0b",3),{}};
   if(req.method=="GET" && req.url.find("/export")!=std::string::npos)
     return luth_http_response{200,"document-data",{}};
   if(req.method=="DELETE")return luth_http_response{204,"",{}};
   return luth_http_response{200,R"({"id":"file_1","name":"example","files":[]})",{}};
 };
 google_drive_client client(transport,[&](){return token;});
 const auto files=client.list_files("name contains 'Música'","page/A+B",10);
 CHECK(files.at("files").as<json_value::array>().empty());
 CHECK(requests.back().url.find("https://www.googleapis.com/drive/v3/files?")==0);
 CHECK(requests.back().url.find("pageToken=page%2FA%2BB")!=std::string::npos);
 CHECK(requests.back().url.find("M%C3%BAsica")!=std::string::npos);
 CHECK(requests.back().headers.at("Authorization")=="Bearer test_token");
 CHECK(!requests.back().credentials_include&&requests.back().no_store);
 CHECK(client.get_metadata("file_1").at("id").as<std::string>()=="file_1");
 CHECK(client.download_media("file_1")==std::string("a\0b",3));
 CHECK(client.export_file("file_1","text/plain")=="document-data");
 CHECK(requests.back().url.find("mimeType=text%2Fplain")!=std::string::npos);
 CHECK(client.create_metadata("Música 😀","text/plain",{"folder_2"}).at("id").as<std::string>()=="file_1");
 CHECK(requests.back().method=="POST");
 CHECK(requests.back().headers.at("Content-Type")=="application/json");
 CHECK(parse_json(requests.back().body).as<json_value::object>().at("parents").as<json_value::array>().size()==1);
 CHECK(client.upload_media(std::string("a\0b",3),"application/octet-stream").at("id").as<std::string>()=="file_1");
 CHECK(requests.back().url.find("/upload/drive/v3/files?uploadType=media")!=std::string::npos);
 CHECK(requests.back().body==std::string("a\0b",3));
 CHECK(client.update_media("file_1","new bytes","text/plain").at("id").as<std::string>()=="file_1");
 CHECK(requests.back().method=="PATCH");
 CHECK(requests.back().url.find("/upload/drive/v3/files/file_1?uploadType=media")!=std::string::npos);
 CHECK(client.rename_file("file_1","new name").at("id").as<std::string>()=="file_1");
 CHECK(requests.back().method=="PATCH");
 CHECK(client.copy_file("file_1","copy").at("id").as<std::string>()=="file_1");
 CHECK(requests.back().method=="POST");
 client.delete_file("file_1");CHECK(requests.back().method=="DELETE");
 const auto count=requests.size();
 CHECK(throws([&](){client.get_metadata("../secret");}));
 CHECK(throws([&](){client.download_media("ab%2fcd");}));
 CHECK(throws([&](){client.delete_file("file/path");}));
 CHECK(throws([&](){client.list_files("", "",0);}));
 CHECK(throws([&](){client.create_metadata("\n", "text/plain");}));
 CHECK(requests.size()==count);
 token=""; CHECK(throws([&](){client.get_metadata("file_1");}));CHECK(requests.size()==count);
 token="evil\r\nHost: bad";CHECK(throws([&](){client.get_metadata("file_1");}));CHECK(requests.size()==count);
 token="test_token";deny=true;
 try{client.get_metadata("file_1");CHECK(false);}catch(const google_drive_error& e){CHECK(e.status()==403);CHECK(std::string(e.what()).find("secret")==std::string::npos);}
 CHECK(throws([&](){google_drive_client bad(transport,[&](){return token;},{"https://evil.example",false,1000,1000});}));
 CHECK(throws([&](){google_drive_client bad(transport,[&](){return token;},{"http://localhost.attacker.test:80",true,1000,1000});}));
 CHECK(throws([&](){google_drive_client bad(transport,[&](){return token;},{"http://127.0.0.1:99999",true,1000,1000});}));
 google_drive_client local(transport,[&](){return token;},{"http://127.0.0.1:1234",true,1000,1000});
 CHECK(throws([&](){local.get_metadata("bad\\id");}));
 return 0;
}
