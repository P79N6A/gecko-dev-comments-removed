

{
  let dummyCallback = () => {};
  let mockWebSocket = new MockWebSocketChannel();

  add_test(function test_initalize_offline() {
    Services.io.offline = true;
    do_check_false(MozLoopPushHandler.initialize());
    Services.io.offline = false;
    MozLoopPushHandler.initialize({mockWebSocket: mockWebSocket});
    run_next_test();
  });

  add_test(function test_initalize_missing_chanid() {
    Assert.throws(() => {MozLoopPushHandler.register(null, dummyCallback, dummyCallback)});
    run_next_test();
  });

  add_test(function test_initalize_missing_regcallback() {
    Assert.throws(() => {MozLoopPushHandler.register("chan-1", null, dummyCallback)});
    run_next_test();
  });

  add_test(function test_initalize_missing_notifycallback() {
    Assert.throws(() => {MozLoopPushHandler.register("chan-1", dummyCallback, null)});
    run_next_test();
  });

  add_test(function test_initalize_websocket() {
    MozLoopPushHandler.register(
      "chan-1",
      function(err, url, id) {
        Assert.equal(err, null, "Should return null for success");
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
      },
      mockWebSocket);
  });

  add_test(function test_reconnect_websocket() {
    MozLoopPushHandler.uaID = undefined;
    MozLoopPushHandler.registeredChannels = {}; 
    mockWebSocket.stop();
  });

  add_test(function test_reopen_websocket() {
    MozLoopPushHandler.uaID = undefined;
    MozLoopPushHandler.registeredChannels = {}; 
    mockWebSocket.serverClose();
  });

  add_test(function test_retry_registration() {
    MozLoopPushHandler.uaID = undefined;
    MozLoopPushHandler.registeredChannels = {}; 
    mockWebSocket.initRegStatus = 500;
    mockWebSocket.stop();
  });

  function run_test() {
    setupFakeLoopServer();

    Services.prefs.setCharPref("services.push.serverURL", kServerPushUrl);
    Services.prefs.setCharPref("loop.server", kLoopServerUrl);
    Services.prefs.setIntPref("loop.retry_delay.start", 10); 
    Services.prefs.setIntPref("loop.retry_delay.limit", 20); 

    run_next_test();
  };
}
