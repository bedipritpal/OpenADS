// Test credentials below are public deterministic fixtures, never deployment keys.
#include "network/server.h"
#include "network/client.h"
#include "openads/ace.h"
#include <filesystem>
#if defined(OPENADS_WITH_TLS)
#include "network/tls_transport.h"
#endif
#include <chrono>
#include <ctime>
#include <fstream>
#include <iostream>
#include <vector>
#include <algorithm>
using namespace openads::network;
using Clock=std::chrono::steady_clock;
int main(int argc,char**argv) {
 const bool tls=argc>1&&std::string(argv[1])=="psk";
 Server srv;srv.set_enable_file_func(true);
#if defined(OPENADS_WITH_TLS)
 TlsConfig c,s;if(tls){c.has_psk=true;c.psk_identity="benchmark-public-fixture";for(std::size_t i=0;i<32;++i)c.psk_key[i]=static_cast<std::uint8_t>(i+1);s.psk_store[c.psk_identity]=c.psk_key;if(!srv.set_tls(s))return 1;}
#else
 if(tls)return 2;
#endif
 if(!srv.start("127.0.0.1",0))return 3;
 std::vector<double> handshake;auto cpu=std::clock();auto start=Clock::now();
 for(int i=0;i<50;++i){RemoteConnection rc;auto t0=Clock::now();
#if defined(OPENADS_WITH_TLS)
 if(tls){auto t=connect_tls("127.0.0.1",srv.port(),c);if(!t||!rc.connect_with_transport(std::move(t).value(),".","",""))return 4;}else
#endif
 if(!rc.connect("127.0.0.1",srv.port(),".","",""))return 5;
 handshake.push_back(std::chrono::duration<double,std::milli>(Clock::now()-t0).count());rc.disconnect();}
 std::sort(handshake.begin(),handshake.end());
 std::cout<<"mode="<<(tls?"psk":"tcp")<<" handshakes=50 median_ms="<<handshake[25]<<" p95_ms="<<handshake[47]<<" wall_ms="<<std::chrono::duration<double,std::milli>(Clock::now()-start).count()<<" cpu_ms="<<1000.0*static_cast<double>(std::clock()-cpu)/CLOCKS_PER_SEC<<"\n";
 RemoteConnection rc;
#if defined(OPENADS_WITH_TLS)
 if(tls){auto t=connect_tls("127.0.0.1",srv.port(),c);if(!t||!rc.connect_with_transport(std::move(t).value(),".","",""))return 6;}else
#endif
 if(!rc.connect("127.0.0.1",srv.port(),".","",""))return 7;
 for(int run=0;run<5;++run){start=Clock::now();cpu=std::clock();for(int i=0;i<1000;++i)if(!rc.file_exists("nonexistent-psk-benchmark.dbf"))return 8;
 std::cout<<"mode="<<(tls?"psk":"tcp")<<" run="<<run<<" small_ops=1000 wall_ms="<<std::chrono::duration<double,std::milli>(Clock::now()-start).count()<<" cpu_ms="<<1000.0*static_cast<double>(std::clock()-cpu)/CLOCKS_PER_SEC<<"\n";}
 rc.disconnect();srv.stop();
 // Matched indexed-prefetch workload, byte identity checked at every row.
 namespace fs=std::filesystem;auto dir=fs::current_path()/"psk_bench_rows";std::error_code ec;fs::remove_all(dir,ec);fs::create_directories(dir);
 auto local=dir.string();ADSHANDLE lc=0,tbl=0,index=0;
 if(AdsConnect60(reinterpret_cast<UNSIGNED8*>(local.data()),ADS_LOCAL_SERVER,nullptr,nullptr,0,&lc))return 9;
 UNSIGNED8 name[]="bench.dbf",fields[]="ID,N,6,0;TXT,C,100,0",id[]="ID",txt[]="TXT",bag[]="bench.cdx",tag[]="REV",expr[]="ID";
 if(AdsCreateTable(lc,name,nullptr,ADS_CDX,ADS_ANSI,0,0,0,fields,&tbl))return 10;
 const std::string value=std::string("ABC")+std::string(97,' ');
 for(int i=0;i<150;++i)if(AdsAppendRecord(tbl)||AdsSetDouble(tbl,id,150-i)||AdsSetFieldRaw(tbl,txt,reinterpret_cast<UNSIGNED8*>(const_cast<char*>(value.data())),100)||AdsWriteRecord(tbl))return 11;
 if(AdsCreateIndex61(tbl,bag,tag,expr,nullptr,nullptr,0,0,&index)||AdsCloseTable(tbl)||AdsDisconnect(lc))return 12;
 Server rows;
#if defined(OPENADS_WITH_TLS)
 if(tls&&!rows.set_tls(s))return 13;
 if(tls&&OAds_SetTlsPsk(reinterpret_cast<const UNSIGNED8*>(c.psk_identity.c_str()),c.psk_key.data(),32))return 14;
#endif
 if(!rows.start("127.0.0.1",0))return 15;
 auto uri=std::string(tls?"tls://":"tcp://")+"127.0.0.1:"+std::to_string(rows.port())+"/"+dir.generic_string();ADSHANDLE remote=0;
 if(AdsConnect60(reinterpret_cast<UNSIGNED8*>(uri.data()),ADS_REMOTE_SERVER,nullptr,nullptr,0,&remote)||AdsOpenTable(remote,name,nullptr,ADS_CDX,ADS_ANSI,ADS_SHARED,ADS_COMPATIBLE_LOCKING,0,&tbl)||AdsGetIndexHandle(tbl,tag,&index))return 16;
 for(int run=0;run<5;++run){start=Clock::now();cpu=std::clock();if(AdsGotoTop(index))return 17;
 for(int i=0;i<150;++i){UNSIGNED8 field[256];UNSIGNED32 len=256;if(AdsGetField(tbl,txt,field,&len,0)||len!=100||std::string(reinterpret_cast<char*>(field),100)!=value)return 18;if(i<149&&AdsSkip(index,1))return 19;}
 std::cout<<"mode="<<(tls?"psk":"tcp")<<" run="<<run<<" indexed_prefetch_rows=150 wall_ms="<<std::chrono::duration<double,std::milli>(Clock::now()-start).count()<<" cpu_ms="<<1000.0*static_cast<double>(std::clock()-cpu)/CLOCKS_PER_SEC<<"\n";}
 AdsCloseTable(tbl);AdsDisconnect(remote);rows.stop();
#if defined(OPENADS_WITH_TLS)
 OAds_SetTlsPsk(nullptr,nullptr,0);
#endif
 fs::remove_all(dir,ec);return 0;
}
