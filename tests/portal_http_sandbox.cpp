#include "tools/serverd/http_server.h"
#include "network/server.h"
#include <thread>
#include <chrono>
int main(int argc, char** argv) {
 openads::network::Server wire;
 if(!wire.start("127.0.0.1",16480)) return 1;
 openads::network::Server::SessionInfo s;s.user="test-private";s.open_tables=3;s.open_table_names={"private/a.dbf","private/a.dbf","private/b.dbf"};wire.register_session(s);
 openads::studio::HttpConsole http;http.add_user("testadmin","sandbox-only");
 if(!http.start("127.0.0.1",16481,(argc > 1 ? argv[1] : "."),&wire))return 2;
 std::this_thread::sleep_for(std::chrono::seconds(9));http.stop();wire.stop();
}
