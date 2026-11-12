// Isolated generated two-tag .Z01 real-wire open/close, no customer data.
#include "network/client.h"
#include "network/server.h"
#include "util/open_phase.h"
#include "openads/ace.h"
#include <filesystem>
#include <fstream>
#include <cstring>
#include <chrono>
#include <iostream>
namespace fs=std::filesystem;
int main(int argc,char** argv){
 fs::path dir=fs::temp_directory_path()/"open80-bench-data";fs::remove_all(dir);fs::create_directories(dir);
 auto root=dir.string();ADSHANDLE c=0,t=0,ix=0;
 if(AdsConnect60((UNSIGNED8*)root.c_str(),ADS_LOCAL_SERVER,nullptr,nullptr,0,&c))return 1;
 UNSIGNED8 name[]="Probe.DBF",def[]="K,C,8;V,C,8",field[]="K",value[]="alpha",bag[]="Probe.Z01",tag1[]="KEY",tag2[]="VALUE",expr2[]="V";
 if(AdsCreateTable(c,name,nullptr,ADS_CDX,ADS_ANSI,0,0,0,def,&t))return 2;
 if(AdsAppendRecord(t)||AdsSetString(t,field,value,5)||AdsWriteRecord(t))return 3;
 if(AdsCreateIndex61(t,bag,tag1,field,nullptr,nullptr,ADS_COMPOUND,0,&ix)||AdsCreateIndex61(t,bag,tag2,expr2,nullptr,nullptr,ADS_COMPOUND,0,&ix))return 4;
 if(AdsCloseTable(t)||AdsDisconnect(c))return 5;
 for(int i=0;i<25;++i){std::ofstream f(dir/("neighbour"+std::to_string(i)+".txt"));f<<"synthetic";}
 openads::network::Server srv;if(!srv.start("127.0.0.1",0))return 6;
 openads::network::RemoteConnection conn;if(!conn.connect("127.0.0.1",srv.port(),dir.string()))return 7;
 auto pair=[&] { auto th=conn.open_table("Probe.DBF",1);if(!th)return false;return conn.close_table(th.value().id).has_value();};
 for(int i=0;i<100;i++)if(!pair())return 8;
 const int n=argc>2?std::stoi(argv[2]):3000;
 const bool on=argc>1&&std::string(argv[1])=="on";auto log=fs::temp_directory_path()/"open80-bench.log";fs::remove(log);
 openads::util::open_phase::configure(on?log.string():"");
 auto start=std::chrono::steady_clock::now();for(int i=0;i<n;i++)if(!pair())return 9;
 auto us=std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now()-start).count();openads::util::open_phase::configure("");
 unsigned rows=0;if(on){std::ifstream f(log);std::string l;while(std::getline(f,l))++rows;if(rows!=static_cast<unsigned>(n))return 10;}
 std::cout<<(on?"on":"off")<<" open_close_us="<<us<<" per_pair_us="<<double(us)/n<<" rows="<<rows<<"\n";
 conn.disconnect();srv.stop();fs::remove_all(dir);
}
