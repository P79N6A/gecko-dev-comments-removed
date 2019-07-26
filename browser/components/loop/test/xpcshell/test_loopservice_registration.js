












add_test(function test_register_offline() {
  
  Services.io.offline = true;

  MozLoopService.register().then(() => {
    do_throw("should not succeed when offline");
  }, err => {
    Assert.equal(err, "offline", "should reject with 'offline' when offline");
    Services.io.offline = false;
    run_next_test();
  });
});





add_test(function test_register_websocket_success_loop_server_fail() {
  MozLoopService.register().then(() => {
    do_throw("should not succeed when loop server registration fails");
  }, err => {
    
    
    Assert.equal(err, 404, "Expected no errors in websocket registration");

    let instances = gMockWebSocketChannelFactory.createdInstances;
    Assert.equal(instances.length, 1,
                 "Should create only one instance of websocket");
    Assert.equal(instances[0].uri.prePath, kServerPushUrl,
                 "Should have the url from preferences");
    Assert.equal(instances[0].origin, kServerPushUrl,
                 "Should have the origin url from preferences");
    Assert.equal(instances[0].protocol, "push-notification",
                 "Should have the protocol set to push-notifications");

    gMockWebSocketChannelFactory.resetInstances();

    run_next_test();
  });
});





add_test(function test_register_success() {
  loopServer.registerPathHandler("/registration", (request, response) => {
    response.setStatusLine(null, 200, "OK");
    response.processAsync();
    response.finish();
  });
  MozLoopService.register().then(() => {
    let instances = gMockWebSocketChannelFactory.createdInstances;
    Assert.equal(instances.length, 1,
                 "Should create only one instance of websocket");

    run_next_test();
  }, err => {
    do_throw("shouldn't error on a succesful request");
  });
});

function run_test()
{
  setupFakeLoopServer();

  
  gMockWebSocketChannelFactory.register();

  do_register_cleanup(function() {
    gMockWebSocketChannelFactory.unregister();
    Services.prefs.clearUserPref("loop.hawk-session-token");
  });

  run_next_test();
}
