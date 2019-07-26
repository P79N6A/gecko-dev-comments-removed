


XPCOMUtils.defineLazyModuleGetter(this, "MozLoopService",
                                  "resource:///modules/loop/MozLoopService.jsm");

var server;

const kServerPushUrl = "http://localhost:3456";




add_test(function test_register_offline() {
  
  Services.io.offline = true;

  MozLoopService.register(function(err) {
    Assert.equal(err, "offline", "Expected error of 'offline' to be returned");

    Services.io.offline = false;
    run_next_test();
  });
});





add_test(function test_register_websocket_success_loop_server_fail() {
  MozLoopService.register(function(err) {
    
    
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





add_test(function test_registration_returns_hawk_session_token() {

  var fakeSessionToken = "1bad3e44b12f77a88fe09f016f6a37c42e40f974bc7a8b432bb0d2f0e37e1750";
  Services.prefs.clearUserPref("loop.hawk-session-token");

  server.registerPathHandler("/registration", (request, response) => {
    response.setStatusLine(null, 200, "OK");
    response.setHeader("Hawk-Session-Token", fakeSessionToken, false);
    response.processAsync();
    response.finish();
  });

  MozLoopService.register(function(err) {

    Assert.equal(err, null, "Should not cause an error to be called back on" +
      " an otherwise valid request");

    var hawkSessionPref;
    try {
      hawkSessionPref = Services.prefs.getCharPref("loop.hawk-session-token");
    } catch (ex) {
      
      dump("unexpected exception: " + ex + "\n");
    }
    Assert.equal(hawkSessionPref, fakeSessionToken, "Should store" +
      " Hawk-Session-Token header contents in loop.hawk-session-token pref");

    run_next_test();
  });
});










add_test(function test_register_success() {
  server.registerPathHandler("/registration", (request, response) => {
    response.setStatusLine(null, 200, "OK");
    response.processAsync();
    response.finish();
  });

  MozLoopService.register(function(err) {
    Assert.equal(err, null, "Expected no errors in websocket registration");

    let instances = gMockWebSocketChannelFactory.createdInstances;
    Assert.equal(instances.length, 1,
                 "Should create only one instance of websocket");

    run_next_test();
  });
});

function run_test()
{
  server = new HttpServer();
  server.start(-1);

  
  gMockWebSocketChannelFactory.register();
  Services.prefs.setCharPref("services.push.serverURL", kServerPushUrl);

  Services.prefs.setCharPref("loop.server", "http://localhost:" + server.identity.primaryPort);

  do_register_cleanup(function() {
    gMockWebSocketChannelFactory.unregister();
    Services.prefs.clearUserPref("loop.hawk-session-token");
    server.stop(function() {});
  });

  run_next_test();

}
