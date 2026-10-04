#include "network/server.h"
#include <thread>
#include <chrono>
int main(){openads::network::Server s;s.set_daemon_hardening(true);s.add_credential("testadmin","sandbox-only");if(!s.start("127.0.0.1",16482))return 1;std::this_thread::sleep_for(std::chrono::seconds(12));s.stop();}
