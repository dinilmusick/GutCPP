#include "gut/model_residency.hpp"
#include <atomic>
#include <chrono>
#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <thread>
#include <vector>
#define CHECK(x) do {if(!(x)){std::cerr<<"model_residency_test:"<<__LINE__<<" "<<#x<<"\n";std::exit(1);}}while(false)
int main(){
 gut::model_residency<int> residency;
 std::atomic<int> loads{0},unloads{0},running{0};
 std::vector<std::thread> workers;std::atomic<bool> proceed{false};
 for(int i=0;i<8;++i)workers.emplace_back([&]{
   while(!proceed.load())std::this_thread::yield();
   const int r=residency.run([&]{++loads;std::this_thread::sleep_for(std::chrono::milliseconds(10));return 55;},
     [&](int& value){CHECK(value==55);++unloads;},
     [&](int& value){CHECK(value==55);++running;std::this_thread::sleep_for(std::chrono::milliseconds(30));--running;return value+1;});
   CHECK(r==56);
 });
 proceed=true;
 for(auto& worker:workers)worker.join();
 CHECK(loads==1);CHECK(unloads==1);CHECK(!residency.is_loaded());CHECK(residency.active_users()==0);
 bool operation_failed=false;
 try{
   residency.run([&]{++loads;return 9;},[&](int&){++unloads;},[](int&)->int{throw std::runtime_error("operation failed");});
 }catch(const std::runtime_error& e){operation_failed=std::string(e.what())=="operation failed";}
 CHECK(operation_failed);CHECK(loads==2);CHECK(unloads==2);CHECK(residency.active_users()==0);
 bool load_failed=false;
 try{
   residency.run([]()->int{throw std::runtime_error("load failed");},[](int&){},[](int&){return 0;});
 }catch(const std::runtime_error&){load_failed=true;}
 CHECK(load_failed);CHECK(!residency.is_loaded());CHECK(residency.active_users()==0);
 bool unload_failed=false;
 try{
   residency.run([]{return 1;},[](int&){throw std::runtime_error("unload failed");},[](int&){return 0;});
 }catch(const std::runtime_error& e){unload_failed=std::string(e.what())=="unload failed";}
 CHECK(unload_failed);CHECK(residency.last_cleanup_error()!=nullptr);
 CHECK(!residency.is_loaded());CHECK(residency.active_users()==0);
}
