#include "network/server.h"
#include "network/client.h"
#include "network/tls_transport.h"
#include <atomic>
#include <thread>
#include <iostream>
#include <sys/socket.h>
#include <poll.h>
#include <unistd.h>
using namespace openads::network;
int main(int argc,char**argv) {
 bool replay=argc>1;TlsConfig s,c;c.has_psk=true;c.psk_identity="unit";
 for(int i=0;i<32;++i)c.psk_key[static_cast<std::size_t>(i)]=static_cast<unsigned char>(i+1);s.psk_store[c.psk_identity]=c.psk_key;
 Server srv;srv.set_enable_file_func(true);if(!srv.set_tls(s)||!srv.start("127.0.0.1",0))return 1;
 auto l=listen_tcp({"127.0.0.1",0,16});if(!l)return 2;auto listener=l.value();auto port=socket_local_port(listener).value();
 std::atomic<bool> armed{false},done{false},changed{false};
 std::thread proxy([&]{auto a=accept_one(listener);auto b=connect_tcp("127.0.0.1",srv.port());if(!a||!b)return;auto left=a.value(),right=b.value();
 std::vector<unsigned char> pending;unsigned char buf[32768];
 while(!done){pollfd f[2]={{static_cast<int>(left.handle),POLLIN,0},{static_cast<int>(right.handle),POLLIN,0}};if(poll(f,2,100)<=0)continue;
 for(int side=0;side<2;++side){if(!(f[side].revents&POLLIN))continue;int n=static_cast<int>(recv(f[side].fd,buf,sizeof(buf),0));if(n<=0){done=true;break;}
 if(side==1){::send(static_cast<int>(left.handle),buf,static_cast<std::size_t>(n),MSG_NOSIGNAL);continue;}
 pending.insert(pending.end(),buf,buf+n);
 while(pending.size()>=5){std::size_t len=5+static_cast<std::size_t>((pending[3]<<8)|pending[4]);if(pending.size()<len)break;
 std::vector<unsigned char> record(pending.begin(),pending.begin()+static_cast<long>(len));pending.erase(pending.begin(),pending.begin()+static_cast<long>(len));
 bool attack=armed&&!changed&&record[0]==23;if(attack&&!replay)record.back()^=1;
 ::send(static_cast<int>(right.handle),record.data(),record.size(),MSG_NOSIGNAL);
 if(attack){changed=true;if(replay)::send(static_cast<int>(right.handle),record.data(),record.size(),MSG_NOSIGNAL);}
 }
 }}sock_close(left);sock_close(right);});
 int result=0;auto t=connect_tls("127.0.0.1",port,c);if(!t)result=3;else{RemoteConnection rc;auto con=rc.connect_with_transport(std::move(t).value(),".","","");if(!con)result=4;else{armed=true;auto r=rc.file_exists("none.dbf");if(replay&&r)r=rc.file_exists("none.dbf");if(r||!changed)result=5;}rc.disconnect();}
 done=true;proxy.join();sock_close(listener);
 auto good=connect_tls("127.0.0.1",srv.port(),c);if(!good)result=6;srv.stop();std::cout<<(replay?"replay":"tamper")<<" result="<<result<<" affected-session-rejected="<<changed<<" next-valid="<<static_cast<bool>(good)<<"\n";return result;
}
