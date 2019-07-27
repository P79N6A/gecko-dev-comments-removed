









#include "webrtc/base/gunit.h"
#include "webrtc/base/httpserver.h"
#include "webrtc/base/testutils.h"

using namespace testing;

namespace rtc {

namespace {
  const char* const kRequest =
    "GET /index.html HTTP/1.1\r\n"
    "Host: localhost\r\n"
    "\r\n";

  struct HttpServerMonitor : public sigslot::has_slots<> {
    HttpServerTransaction* transaction;
    bool server_closed, connection_closed;

    HttpServerMonitor(HttpServer* server)
    : transaction(NULL), server_closed(false), connection_closed(false) {
      server->SignalCloseAllComplete.connect(this,
        &HttpServerMonitor::OnClosed);
      server->SignalHttpRequest.connect(this, &HttpServerMonitor::OnRequest);
      server->SignalHttpRequestComplete.connect(this,
        &HttpServerMonitor::OnRequestComplete);
      server->SignalConnectionClosed.connect(this,
        &HttpServerMonitor::OnConnectionClosed);
    }
    void OnRequest(HttpServer*, HttpServerTransaction* t) {
      ASSERT_FALSE(transaction);
      transaction = t;
      transaction->response.set_success();
      transaction->response.setHeader(HH_CONNECTION, "Close");
    }
    void OnRequestComplete(HttpServer*, HttpServerTransaction* t, int) {
      ASSERT_EQ(transaction, t);
      transaction = NULL;
    }
    void OnClosed(HttpServer*) {
      server_closed = true;
    }
    void OnConnectionClosed(HttpServer*, int, StreamInterface* stream) {
      connection_closed = true;
      delete stream;
    }
  };

  void CreateClientConnection(HttpServer& server,
                              HttpServerMonitor& monitor,
                              bool send_request) {
    StreamSource* client = new StreamSource;
    client->SetState(SS_OPEN);
    server.HandleConnection(client);
    EXPECT_FALSE(monitor.server_closed);
    EXPECT_FALSE(monitor.transaction);

    if (send_request) {
      
      client->QueueString(kRequest);
      EXPECT_FALSE(monitor.server_closed);
    }
  }
}  

TEST(HttpServer, DoesNotSignalCloseUnlessCloseAllIsCalled) {
  HttpServer server;
  HttpServerMonitor monitor(&server);
  
  CreateClientConnection(server, monitor, true);
  
  ASSERT_TRUE(NULL != monitor.transaction);
  server.Respond(monitor.transaction);
  EXPECT_FALSE(monitor.transaction);
  
  EXPECT_FALSE(monitor.server_closed);
  EXPECT_TRUE(monitor.connection_closed);
}

TEST(HttpServer, SignalsCloseWhenNoConnectionsAreActive) {
  HttpServer server;
  HttpServerMonitor monitor(&server);
  
  CreateClientConnection(server, monitor, false);
  
  server.CloseAll(false);
  
  EXPECT_TRUE(monitor.server_closed);
  EXPECT_TRUE(monitor.connection_closed);
}

TEST(HttpServer, SignalsCloseAfterGracefulCloseAll) {
  HttpServer server;
  HttpServerMonitor monitor(&server);
  
  CreateClientConnection(server, monitor, true);
  
  server.CloseAll(false);
  EXPECT_FALSE(monitor.server_closed);
  
  ASSERT_TRUE(NULL != monitor.transaction);
  server.Respond(monitor.transaction);
  EXPECT_FALSE(monitor.transaction);
  
  EXPECT_TRUE(monitor.server_closed);
  EXPECT_TRUE(monitor.connection_closed);
}

TEST(HttpServer, SignalsCloseAfterForcedCloseAll) {
  HttpServer server;
  HttpServerMonitor monitor(&server);
  
  CreateClientConnection(server, monitor, true);
  
  server.CloseAll(true);
  
  EXPECT_TRUE(monitor.server_closed);
  EXPECT_TRUE(monitor.connection_closed);
}

} 
