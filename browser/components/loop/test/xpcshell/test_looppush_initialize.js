

{
  let dummyCallback = () => {};
  let mockWebSocket = new MockWebSocketChannel();
  let pushServerRequestCount = 0;

  add_test(function test_initalize_offline() {
    Services.io.offline = true;
    do_check_false(MozLoopPushHandler.initialize());
    Services.io.offline = false;
    run_next_test();
  });

  add_test(function test_initalize_missing_chanid() {
    Assert.throws(() => MozLoopPushHandler.register(null, dummyCallback, dummyCallback));
    run_next_test();
  });

  add_test(function test_initalize_missing_regcallback() {
    Assert.throws(() => MozLoopPushHandler.register("chan-1", null, dummyCallback));
    run_next_test();
  });

  add_test(function test_initalize_missing_notifycallback() {
    Assert.throws(() => MozLoopPushHandler.register("chan-1", dummyCallback, null));
    run_next_test();
  });

  add_test(function test_initalize_websocket() {
    do_check_true(MozLoopPushHandler.initialize({mockWebSocket: mockWebSocket}));
    MozLoopPushHandler.register(
      "chan-1",
      function(err, url, id) {
        Assert.equal(err, null, "err should be null to indicate success");
        Assert.equal(url, kEndPointUrl, "Should return push server application URL");
        Assert.equal(id, "chan-1", "Should have channel id = chan-1");
        Assert.equal(mockWebSocket.uri.prePath, kServerPushUrl,
                     "Should have the url from preferences");
        Assert.equal(mockWebSocket.origin, kServerPushUrl,
                     "Should have the origin url from preferences");
        Assert.equal(mockWebSocket.protocol, "push-notification",
                     "Should have the protocol set to push-notifications");
        mockWebSocket.notify(15);
      },
      function(version, id) {
        Assert.equal(version, 15, "Should have version number 15");
        Assert.equal(id, "chan-1", "Should have channel id = chan-1");
        run_next_test();
      });
  });

  add_test(function test_register_twice_same_channel() {
    MozLoopPushHandler.register(
      "chan-2",
      function(err, url, id) {
        Assert.equal(err, null, "Should return null for success");
        Assert.equal(url, kEndPointUrl, "Should return push server application URL");
        Assert.equal(id, "chan-2", "Should have channel id = chan-2");
        Assert.equal(mockWebSocket.uri.prePath, kServerPushUrl,
                     "Should have the url from preferences");
        Assert.equal(mockWebSocket.origin, kServerPushUrl,
                     "Should have the origin url from preferences");
        Assert.equal(mockWebSocket.protocol, "push-notification",
                     "Should have the protocol set to push-notifications");

        
        MozLoopPushHandler.register(
          "chan-2",
          function(err, url, id) {
            Assert.notEqual(err, null, "Should have returned an error");
            
            mockWebSocket.notify(16);
          },
          function(version, id) {
            Assert.ok(false, "The 2nd onNotification callback shouldn't be called");
        });
      },
      function(version, id) {
        Assert.equal(version, 16, "Should have version number 16");
        Assert.equal(id, "chan-2", "Should have channel id = chan-2");
        run_next_test();
      });
  });

  
  
  add_test(function test_reconnect_websocket() {
    MozLoopPushHandler.uaID = undefined;
    mockWebSocket.stop();
    
  });

  
  
  add_test(function test_reopen_websocket() {
    MozLoopPushHandler.uaID = undefined;
    MozLoopPushHandler.registeredChannels = {}; 
    mockWebSocket.serverClose();
    
  });

  
  
  add_test(function test_retry_registration() {
    MozLoopPushHandler.uaID = undefined;
    mockWebSocket.initRegStatus = 500;
    mockWebSocket.stop();
  });

  add_test(function test_reconnect_no_registration() {
    let regCnt = 0;
    MozLoopPushHandler.shutdown();
    MozLoopPushHandler.initialize({mockWebSocket: mockWebSocket});
    MozLoopPushHandler.register(
      "test-chan",
      function(err, url, id) {
        Assert.equal(++regCnt, 1, "onRegistered should only be called once");
        Assert.equal(err, null, "err should be null to indicate success");
        Assert.equal(url, kEndPointUrl, "Should return push server application URL");
        Assert.equal(id, "test-chan", "Should have channel id = test-chan");
        mockWebSocket.stop();
        setTimeout(run_next_test(), 0);
      },
      function(version, id) {
        return;
      });
  });

  add_test(function test_ping_websocket() {
    let pingReceived = false,
        socketClosed = false;
    mockWebSocket.defaultMsgHandler = (msg) => {
      pingReceived = true;
      
    }
    mockWebSocket.close = () => {
      socketClosed = true;
    }

    MozLoopPushHandler.shutdown();
    MozLoopPushHandler.initialize({mockWebSocket: mockWebSocket});
    MozLoopPushHandler.register(
      "test-chan",
      function(err, url) {
        Assert.equal(err, null, "err should be null to indicate success");
        waitForCondition(() => pingReceived).then(() => {
          waitForCondition(() => socketClosed).then(() => {
            run_next_test();
          }, () => {
            do_throw("should have closed the websocket");
          });
        }, () => {
          do_throw("should have sent ping");
        });
      },
      function(version) {
        return;
      });
  });

  add_test(function test_retry_pushurl() {
    MozLoopPushHandler.shutdown();
    loopServer.registerPathHandler("/push-server-config", (request, response) => {
      
      
      let n = 0;
      switch (++pushServerRequestCount) {
      case ++n:
        
        response.setStatusLine(null, 500, "Retry");
        response.processAsync();
        response.finish();
        break;
      case ++n:
        
        response.setStatusLine(null, 200, "OK");
        response.write(JSON.stringify({pushServerURI: null}));
        response.processAsync();
        response.finish();
        break;
      case ++n:
        
        response.setStatusLine(null, 200, "OK");
        response.processAsync();
        response.finish();
        break;
      case ++n:
        response.setStatusLine(null, 200, "OK");
        response.write(JSON.stringify({pushServerURI: kServerPushUrl}));
        response.processAsync();
        response.finish();

        run_next_test();
        break;
      }
    });

    do_check_true(MozLoopPushHandler.initialize({mockWebSocket: mockWebSocket}));
  });

  function run_test() {
    setupFakeLoopServer();

    loopServer.registerPathHandler("/push-server-config", (request, response) => {
      response.setStatusLine(null, 200, "OK");
      response.write(JSON.stringify({pushServerURI: kServerPushUrl}));
      response.processAsync();
      response.finish();
    });

    Services.prefs.setCharPref("services.push.serverURL", kServerPushUrl);
    Services.prefs.setIntPref("loop.retry_delay.start", 10); 
    Services.prefs.setIntPref("loop.retry_delay.limit", 20); 
    Services.prefs.setIntPref("loop.ping.interval", 50); 
    Services.prefs.setIntPref("loop.ping.timeout", 20); 

    do_register_cleanup(function() {
      Services.prefs.clearUserPref("services.push.serverULR");
      Services.prefs.clearUserPref("loop.retry_delay.start");
      Services.prefs.clearUserPref("loop.retry_delay.limit");
      Services.prefs.clearUserPref("loop.ping.interval");
      Services.prefs.clearUserPref("loop.ping.timeout");
    });

    run_next_test();
  };
}
