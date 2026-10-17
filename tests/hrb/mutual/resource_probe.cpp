#include "network/server.h"
#include "network/client.h"
#include "network/tls_transport.h"
#include <cassert>
#include <chrono>
#include <filesystem>
#include <iostream>
#include <thread>
using namespace openads::network;
int main(int argc,char**argv) {
 assert(argc==3);
 auto root=std::filesystem::current_path()/"resource-proof";
 std::filesystem::create_directories(root);
 auto worker=std::make_shared<AuthWorker>(argv[1],argv[2]);
 assert(worker->probe());
 auto cfg=make_auth_tls_config();assert(cfg);
 Server daemon;daemon.set_daemon_hardening(true);daemon.set_data_dir(root.string());
 daemon.add_credential("usr","pw");daemon.set_auth_worker(worker);
 assert(daemon.set_tls(cfg.value()));assert(daemon.start("127.0.0.1",0));
 auto tls=cfg.value();tls.insecure_skip_verify=true;tls.auth_tls13_only=true;
 auto attempt=[&](const std::string& blob,bool allow) {
  auto transport=connect_tls("127.0.0.1",daemon.port(),tls);assert(transport);
  RemoteConnection client;
  auto begun=client.begin_auth(std::move(transport).value(),root.string());
  if(!begun)std::cerr<<"begin failed: "<<begun.error().message<<" context="<<begun.error().context<<"\n";
  assert(begun);
  auto start=std::chrono::steady_clock::now();
  auto done=client.complete_auth(blob);
  assert(bool(done)==allow);
  assert(std::chrono::steady_clock::now()-start<std::chrono::seconds(4));
  if(allow)assert(client.server_version().find("openads/")==0);
 };
 unsigned backoff=1;
 for(const auto* attack:{"huge-string","huge-array","spin","runtime-error"}) {
  std::cerr<<"testing "<<attack<<std::endl;
  attempt(attack,false);
  assert(worker->probe());
  // Existing IP failure backoff is intentional. Hello must still answer now.
  auto health=connect_tls("127.0.0.1",daemon.port(),tls);assert(health);
  Frame hello;hello.opcode=Opcode::Hello;assert(write_frame(*health.value(),hello));
  auto reply=read_frame(*health.value());assert(reply&&reply.value().opcode==Opcode::HelloAck);
  std::this_thread::sleep_for(std::chrono::milliseconds(backoff*1000+100));backoff*=2;
  attempt("ok",true);
  std::cout<<attack<<": refused; worker ready; same daemon authenticated next session\n";
 }
 daemon.stop();std::filesystem::remove_all(root);
}
