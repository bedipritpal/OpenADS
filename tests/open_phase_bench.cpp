// Synthetic real-wire opens, bypassing ABI parked-handle reuse. No customer data.
#include "network/client.h"
#include "network/server.h"
#include "util/open_phase.h"
#include <filesystem>
#include <fstream>
#include <cstring>
#include <chrono>
#include <iostream>
namespace fs=std::filesystem;
int main(int argc,char** argv){
 fs::path dir=fs::temp_directory_path()/"open79-bench-data";fs::create_directories(dir);
 unsigned char v[66]{};v[0]=3;v[8]=65;v[10]=5;std::memcpy(v+32,"ID",2);v[43]='C';v[48]=4;v[64]=13;v[65]=26;
 std::ofstream f(dir/"private_name.dbf",std::ios::binary);f.write((char*)v,66);f.close();
 openads::network::Server srv;if(!srv.start("127.0.0.1",0))return 4;
 openads::network::RemoteConnection c;
 if(!c.connect("127.0.0.1",srv.port(),dir.string()))return 5;
 auto pair=[&] { auto t=c.open_table("private_name.dbf");if(!t)return false;return c.close_table(t.value().id).has_value();};
 for(int i=0;i<100;i++)if(!pair())return 6;
 const int n=argc>2?std::stoi(argv[2]):2000;
 const bool on=argc>1 && std::string(argv[1])=="on";
 openads::util::open_phase::configure(on?(fs::temp_directory_path()/"open79-bench.log").string():"");
 auto start=std::chrono::steady_clock::now();
 for(int i=0;i<n;i++)if(!pair())return 7;
 auto us=std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now()-start).count();
 openads::util::open_phase::configure("");
 std::cout<<(on?"on":"off")<<" open_close_us="<<us<<" per_pair_us="<<double(us)/n<<"\n";
 c.disconnect();srv.stop();
}
