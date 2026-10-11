// Pritpal Bedi's WAN padding regression: indexed RDD rows only.
#include "doctest.h"
#include "openads/ace.h"
#include "network/server.h"
#include "network/client.h"
#include "mgmt/mg_stats.h"
#include <chrono>
#include <iostream>
#include <filesystem>
#include <cstring>
#include <string>
#include <vector>
#include <algorithm>
namespace {
std::string wp_value(int i) {
    std::string s(100, ' ');
    switch(i % 5) {
    case 0: break;
    case 1: s.replace(0, 5, "ABC  "); break;
    case 2: s[0]=' '; s[1]='X'; s[2]=0; s[3]=static_cast<char>(0xFF); s[4]=' '; break;
    case 3: for(int k=0;k<100;++k) s[static_cast<std::size_t>(k)]=static_cast<char>((i+k)%256); s[98]=' '; s[99]=' '; break;
    case 4: s[0]=0; s[99]=0; break;
    }
    return s;
}
}
TEST_CASE("indexed fixed Character wire compactness preserves exact bytes and raw records") {
    namespace fs=std::filesystem;
    auto dir=fs::temp_directory_path()/"openads_index_char_padding";
    std::error_code ec; fs::remove_all(dir,ec); fs::create_directories(dir);
    auto path=dir.string(); std::vector<UNSIGNED8> local(path.begin(),path.end());local.push_back(0);
    ADSHANDLE lc=0,t=0,idx=0;
    REQUIRE(AdsConnect60(local.data(),ADS_LOCAL_SERVER,nullptr,nullptr,0,&lc)==0);
    UNSIGNED8 name[]="pad.dbf",def[]="ID,N,6,0;TXT,C,100,0",field[]="TXT",id[]="ID";
    REQUIRE(AdsCreateTable(lc,name,nullptr,ADS_CDX,ADS_ANSI,0,0,0,def,&t)==0);
    constexpr int count=150;
    std::vector<std::vector<UNSIGNED8>> raw;
    for(int i=1;i<=count;++i) {
        REQUIRE(AdsAppendRecord(t)==0); REQUIRE(AdsSetDouble(t,id,count-i+1)==0);
        auto val=wp_value(i);
        REQUIRE(AdsSetFieldRaw(t,field,reinterpret_cast<UNSIGNED8*>(val.data()),100)==0);
        REQUIRE(AdsWriteRecord(t)==0);
        UNSIGNED8 buf[256]{};UNSIGNED32 len=sizeof(buf);
        REQUIRE(AdsGetRecord(t,buf,&len)==0); raw.emplace_back(buf,buf+len);
    }
    UNSIGNED8 bag[]="pad.cdx",tag[]="REV",expr[]="ID";
    REQUIRE(AdsCreateIndex61(t,bag,tag,expr,nullptr,nullptr,0,0,&idx)==0);
    REQUIRE(AdsCloseTable(t)==0);
    UNSIGNED8 sqlname[]="sqlpad.dbf", sqldef[]="TXT,C,100,0";
    REQUIRE(AdsCreateTable(lc,sqlname,nullptr,ADS_CDX,ADS_ANSI,0,0,0,sqldef,&t)==0);
    REQUIRE(AdsAppendRecord(t)==0); REQUIRE(AdsWriteRecord(t)==0);
    REQUIRE(AdsCloseTable(t)==0);REQUIRE(AdsDisconnect(lc)==0);
    openads::network::Server srv;REQUIRE(srv.start("127.0.0.1",0).has_value());
    std::string uri="tcp://127.0.0.1:"+std::to_string(srv.port())+"/"+dir.generic_string();
    std::vector<UNSIGNED8> remote(uri.begin(),uri.end());remote.push_back(0);
    ADSHANDLE rc=0,rt=0,ri=0;
    REQUIRE(AdsConnect60(remote.data(),ADS_REMOTE_SERVER,nullptr,nullptr,0,&rc)==0);
    REQUIRE(AdsOpenTable(rc,name,nullptr,ADS_CDX,ADS_ANSI,ADS_SHARED,ADS_COMPATIBLE_LOCKING,0,&rt)==0);
    REQUIRE(AdsGetIndexHandle(rt,tag,&ri)==0);
    auto check=[&](int n, bool check_raw) {
        UNSIGNED32 rn=0;REQUIRE(AdsGetRecordNum(rt,0,&rn)==0);REQUIRE(rn==static_cast<UNSIGNED32>(n));
        UNSIGNED8 buf[256]{};UNSIGNED32 len=sizeof(buf);
        REQUIRE(AdsGetField(rt,field,buf,&len,0)==0);CHECK(len==100);
        CHECK(std::string(reinterpret_cast<char*>(buf),len)==wp_value(n));
        if(check_raw) {len=sizeof(buf);REQUIRE(AdsGetRecord(rt,buf,&len)==0);CHECK(std::vector<UNSIGNED8>(buf,buf+len)==raw[static_cast<std::size_t>(n-1)]);}
    };
    auto &stats=openads::mgmt::process_mg_stats();
    const auto bytes_before=stats.bytes_out.load();
    const auto clock_before=std::chrono::steady_clock::now();
    REQUIRE(AdsGotoTop(ri)==0);
    for(int n=count;n>=1;--n) {check(n,false);if(n>1)REQUIRE(AdsSkip(ri,1)==0);}
    REQUIRE(AdsGotoBottom(ri)==0);
    for(int n=1;n<=count;++n) {check(n,false);if(n<count)REQUIRE(AdsSkip(ri,-1)==0);}
    std::cout << "PADDING_SCAN bytes=" << stats.bytes_out.load()-bytes_before
              << " us=" << std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now()-clock_before).count() << "\n";
    REQUIRE(AdsGotoTop(ri)==0);check(count,true);REQUIRE(AdsSkip(ri,9)==0);check(count-9,true);
    REQUIRE(AdsCloseTable(rt)==0);REQUIRE(AdsDisconnect(rc)==0);
    // Direct client exposes the transmitted value, before ABI reconstruction.
    openads::network::RemoteConnection conn;
    REQUIRE(conn.connect("127.0.0.1",srv.port(),dir.generic_string(),"","").has_value());
    auto opened=conn.open_table("pad.dbf");REQUIRE(opened.has_value());
    auto indexes=conn.open_index(opened.value().id,"pad.cdx");REQUIRE(indexes.has_value());
    auto row=conn.fetch_current_row(opened.value().id);REQUIRE(row.has_value());
    REQUIRE(row.value().has_row);REQUIRE(row.value().fields.size()==2);
    REQUIRE(conn.set_order_by_name(opened.value().id,"REV").has_value());
    REQUIRE(conn.goto_top(opened.value().id).has_value());
    for(int n=count;n>=count-4;--n) {
        row=conn.fetch_current_row(opened.value().id);REQUIRE(row.has_value());
        auto expected=wp_value(n);
        while(!expected.empty() && expected.back()==' ')expected.pop_back();
        CHECK(row.value().fields[1]==expected);
        REQUIRE(conn.skip(opened.value().id,1).has_value());
    }
    conn.disconnect();srv.stop();fs::remove_all(dir,ec);
}
