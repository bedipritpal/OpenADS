// Disposable native old/new resolver parity and measurement, no customer data.
#include "platform/path.h"
#include "util/open_phase.h"
#include <algorithm>
#include <atomic>
#include <chrono>
#include <cctype>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>
namespace fs=std::filesystem;
std::string to_lower(std::string s){std::transform(s.begin(),s.end(),s.begin(),[](unsigned char c){return static_cast<char>(std::tolower(c));});return s;}
std::string safe_path_string(const fs::path&p){try{return p.string();}catch(const std::exception&){return {};}}
std::string legacy_resolve(const std::string& path) {
    openads::util::open_phase::Detail case_timer(openads::util::open_phase::Metric::Case);
    fs::path p(path);
    fs::path parent = p.parent_path();
    if (parent.empty()) parent = ".";

    std::error_code ec;
    if (!fs::is_directory(parent, ec)) {
        // Parent directory does not exist: nothing to resolve.
        return path;
    }

    const std::string leaf      = p.filename().string();
    const std::string leaf_low  = to_lower(leaf);

    // First pass: prefer an exact-case match if one exists. This keeps
    // the input verbatim when the case is already correct.
    openads::util::open_phase::count(openads::util::open_phase::Metric::DirPass);
    for (const auto& entry : fs::directory_iterator(parent, ec)) {
        openads::util::open_phase::count(openads::util::open_phase::Metric::DirEntry);
        if (ec) break;
        const std::string name = safe_path_string(entry.path().filename());
        if (name.empty()) continue;   // unconvertible neighbour: skip it
        if (name == leaf) {
            return safe_path_string(entry.path());
        }
    }

    // Second pass: case-insensitive match returns the on-disk casing.
    openads::util::open_phase::count(openads::util::open_phase::Metric::DirPass);
    for (const auto& entry : fs::directory_iterator(parent, ec)) {
        openads::util::open_phase::count(openads::util::open_phase::Metric::DirEntry);
        if (ec) break;
        const std::string name = safe_path_string(entry.path().filename());
        if (name.empty()) continue;   // unconvertible neighbour: skip it
        if (to_lower(name) == leaf_low) {
            return safe_path_string(entry.path());
        }
    }

    return path;
}

void check(bool b,const char*msg){if(!b)throw std::runtime_error(msg);}
std::string outcome(const std::string&p,bool old){try{return "ok:"+(old?legacy_resolve(p):openads::platform::resolve_case_insensitive(p));}catch(const fs::filesystem_error&e){return "fs:"+std::to_string(e.code().value());}}
void parity(const fs::path&p){check(outcome(p.string(),true)==outcome(p.string(),false),"stable old/new result/error mismatch");}
int main(int argc,char**argv){try{
 auto nonce=std::to_string(std::chrono::steady_clock::now().time_since_epoch().count());auto root=fs::temp_directory_path()/("open81-parity-"+nonce);fs::create_directories(root);auto dir=root/"data";fs::create_directories(dir);
 for(int i=0;i<400;++i){std::ofstream f(dir/("neighbor"+std::to_string(i)+".tmp"));f<<"generated";}
 {std::ofstream f(dir/"Clientes.DBF");f<<"first";}
 parity(dir/"Clientes.DBF");parity(dir/"clientes.dbf");parity(dir/"missing.cdx");parity(root/"absent"/"missing.dbf");parity(root/"data"/".."/"data"/"clientes.dbf");
 {std::ofstream f(dir/"not-a-directory");f<<"x";}parity(dir/"not-a-directory"/"file.dbf");
 // Exact spelling must beat an earlier folded match; duplicate-folded
 // ambiguity without exact spelling falls back to the original algorithm.
 {std::ofstream f(dir/"DUP.dbf");f<<"upper";}
 bool case_sensitive=!fs::exists(dir/"dup.dbf");
 if(case_sensitive){std::ofstream f(dir/"dup.dbf");f<<"lower";f.close();parity(dir/"DUP.dbf");parity(dir/"dup.dbf");parity(dir/"DuP.DbF");}
 // Each fresh call sees create/delete/recreate and extension changes.
 auto q=dir/"fresh.dbf";parity(q);{std::ofstream f(dir/"Fresh.DBF");f<<"new";}parity(q);fs::remove(dir/"Fresh.DBF");parity(q);{std::ofstream f(dir/"Fresh.DBF");f<<"replacement";}parity(q);
 for(auto ext:{".cdx",".z01",".adi"}){auto p=dir/(std::string("BAG")+ext);{std::ofstream f(p);f<<"fixture";}parity(dir/(std::string("bag")+ext));}
 // Odd names: invalid byte neighbour on POSIX, UTF16 PUA on Windows.
#ifdef _WIN32
 std::wstring odd=L"trap_";odd.push_back(static_cast<wchar_t>(0xF03A));odd+=L".tmp";auto trap=dir/odd;
#else
 auto trap=dir/std::string("trap_\xff.tmp");
#endif
 {std::ofstream f(trap);f<<"x";}parity(dir/"clientes.dbf");parity(dir/"missing2.dbf");
 // Permission denial uses the same externally visible result/error as80.
 auto denied=root/"denied";fs::create_directories(denied);{std::ofstream f(denied/"X.dbf");f<<"x";}
 bool permission_test=false;
#ifndef _WIN32
 fs::permissions(denied,fs::perms::none);std::error_code ec;fs::directory_iterator test(denied,ec);permission_test=bool(ec);parity(denied/"x.dbf");fs::permissions(denied,fs::perms::owner_all);
#endif
 // Concurrent file replacement plus readers. Neither implementation offers
 // an atomic directory snapshot: compare admissible outcomes, not schedules.
 auto race=dir/"Race.DBF";{std::ofstream f(race);f<<"a";}std::atomic<bool> go{true};std::atomic<unsigned> errors{0};
 std::thread writer([&]{try{for(int i=0;i<300;++i){auto tmp=dir/"replacement.tmp";{std::ofstream f(tmp);f<<i;}std::error_code ec;fs::remove(race,ec);fs::rename(tmp,race,ec);}}catch(...){++errors;}go=false;});
 std::vector<std::thread> readers;for(int n=0;n<8;++n)readers.emplace_back([&]{try{for(int i=0;i<300||go;++i){for(bool old:{true,false}){auto v=outcome((dir/"race.dbf").string(),old);if(v!="ok:"+(dir/"race.dbf").string()&&v!="ok:"+race.string())++errors;}}}catch(...){++errors;}});
 writer.join();for(auto&t:readers)t.join();check(errors==0,"replacement/concurrent lookup invariant failed");parity(dir/"race.dbf");
 // Native fixture lookup latency only; same directory/input for old/new.
 int loops=argc>1?std::stoi(argv[1]):1000;
 for(auto query:{"Clientes.DBF","clientes.dbf","absent.cdx"}){
  std::vector<double> times[2];for(int trial=0;trial<7;++trial)for(int order=0;order<2;++order){int which=(order+trial)%2;auto start=std::chrono::steady_clock::now();std::string value;for(int i=0;i<loops;++i)value=which?openads::platform::resolve_case_insensitive((dir/query).string()):legacy_resolve((dir/query).string());auto ns=std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::steady_clock::now()-start).count();times[which].push_back(double(ns)/loops);check(!value.empty(),"lookup empty");}
  for(auto&v:times)std::sort(v.begin(),v.end());std::cout<<"LOOKUP81 query="<<query<<" old_median_ns="<<times[0][3]<<" new_median_ns="<<times[1][3]<<" old_range_ns="<<times[0][0]<<":"<<times[0][6]<<" new_range_ns="<<times[1][0]<<":"<<times[1][6]<<"\n";
 }
 fs::remove_all(root);std::cout<<"PARITY81 stable_results_errors=PASS exact_preference=PASS duplicate_case="<<(case_sensitive?"PASS":"UNAVAILABLE")<<" odd_neighbor=PASS freshness=PASS concurrent_replacement=PASS readers=8 permissions="<<(permission_test?"PASS":"UNAVAILABLE")<<"\n";return 0;
 }catch(const std::exception&e){std::cerr<<"PARITY81 FAIL "<<e.what()<<"\n";return 1;}}
