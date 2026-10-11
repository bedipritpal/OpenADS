#include "doctest.h"
#include "openads/ace.h"
#include "network/server.h"
#include "network/client.h"
#if defined(OPENADS_WITH_TLS)
#include "network/tls_transport.h"
#include <thread>

TEST_CASE("TLS1.3 PSK ephemeral authenticates and contains identity/key failures") {
    using namespace openads::network;
    TlsConfig server, client;
    client.has_psk = true; client.psk_identity = "unit-only";
    for (std::size_t i=0; i<32; ++i) client.psk_key[i] = static_cast<std::uint8_t>(i+1);
    server.psk_store[client.psk_identity] = client.psk_key;
    Server srv; REQUIRE(srv.set_tls(server)); REQUIRE(srv.start("127.0.0.1",0));
    auto uri=std::string("tls://127.0.0.1:")+std::to_string(srv.port())+"/.";
    REQUIRE(OAds_SetTlsPsk(reinterpret_cast<const UNSIGNED8*>(client.psk_identity.c_str()),client.psk_key.data(),32)==0);
    ADSHANDLE h=0;
    REQUIRE(AdsConnect60(reinterpret_cast<UNSIGNED8*>(uri.data()),0,nullptr,nullptr,0,&h)==0);
    CHECK(h!=0);
    // New threads never inherit another thread's secret.
    UNSIGNED32 other_status = 0;
    std::thread other([&] {
        ADSHANDLE other_handle=0;
        other_status=AdsConnect60(reinterpret_cast<UNSIGNED8*>(uri.data()),0,nullptr,nullptr,0,&other_handle);
        if (other_handle) AdsDisconnect(other_handle);
    });
    other.join(); CHECK(other_status!=0);
    ADSHANDLE still_configured=0;
    REQUIRE(AdsConnect60(reinterpret_cast<UNSIGNED8*>(uri.data()),0,nullptr,nullptr,0,&still_configured)==0);
    CHECK(AdsDisconnect(still_configured)==0);
    CHECK(OAds_SetTlsPsk(nullptr,nullptr,0)==0);
    ADSHANDLE cleared=0;
    CHECK(AdsConnect60(reinterpret_cast<UNSIGNED8*>(uri.data()),0,nullptr,nullptr,0,&cleared)!=0);
    CHECK(cleared==0);
    // Clearing cannot tear down an already-established connection.
    CHECK(AdsDisconnect(h)==0);
    auto bad=client; bad.psk_key[0]^=1;
    CHECK_FALSE(connect_tls("127.0.0.1",srv.port(),bad));
    bad=client; bad.psk_identity="unknown";
    CHECK_FALSE(connect_tls("127.0.0.1",srv.port(),bad));
    // A legacy plaintext client receives refusal, not a downgrade.
    RemoteConnection plain;
    CHECK_FALSE(plain.connect("127.0.0.1",srv.port(),".","",""));
    auto good=connect_tls("127.0.0.1",srv.port(),client); REQUIRE(good);
    RemoteConnection rc;
    REQUIRE(rc.connect_with_transport(std::move(good).value(),".","",""));
    rc.disconnect();
    // Setter validates shape without logging or accepting password-sized keys.
    CHECK(OAds_SetTlsPsk(reinterpret_cast<const UNSIGNED8*>("id"),client.psk_key.data(),31)!=0);
    CHECK(OAds_SetTlsPsk(nullptr,client.psk_key.data(),32)!=0);
    CHECK(OAds_SetTlsPsk(reinterpret_cast<const UNSIGNED8*>(""),client.psk_key.data(),32)!=0);
    CHECK(OAds_SetTlsPsk(nullptr,nullptr,0)==0);
    srv.stop();
}
#else
TEST_CASE("PSK setter fails explicitly in TLS-disabled build") {
    CHECK(OAds_SetTlsPsk(nullptr,nullptr,0)!=0);
}
#endif
