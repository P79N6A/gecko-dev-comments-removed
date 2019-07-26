


XPCOMUtils.defineLazyModuleGetter(this, "MozLoopService",
                                  "resource:///modules/loop/MozLoopService.jsm");

var server;

const kServerPushUrl = "http://localhost:3456";





add_test(function test_initialize_no_expiry() {
  MozLoopService.initialize(function(err) {
    Assert.equal(err, false,
      "should not register when no expiry time is set");

    run_next_test();
  });
});





add_test(function test_initialize_expiry_past() {
  
  var nowSeconds = Date.now() / 1000;
  Services.prefs.setIntPref("loop.urlsExpiryTimeSeconds", nowSeconds - 2);

  MozLoopService.initialize(function(err) {
    Assert.equal(err, false,
      "should not register when expiry time is in past");

    run_next_test();
  });
});





add_test(function test_initialize_and_register() {
  
  var nowSeconds = Date.now() / 1000;
  Services.prefs.setIntPref("loop.urlsExpiryTimeSeconds", nowSeconds + 60);

  MozLoopService.initialize(function(err) {
    Assert.equal(err, null,
      "should not register when expiry time is in past");

    run_next_test();
  });
});

function run_test()
{
  server = new HttpServer();
  server.start(-1);
  server.registerPathHandler("/registration", (request, response) => {
    response.setStatusLine(null, 200, "OK");
    response.processAsync();
    response.finish();
  });

  
  gMockWebSocketChannelFactory.register();
  Services.prefs.setCharPref("services.push.serverURL", kServerPushUrl);
  Services.prefs.setCharPref("loop.server", "http://localhost:" + server.identity.primaryPort);

  
  Services.prefs.setIntPref("loop.initialDelay", 10);

  do_register_cleanup(function() {
    gMockWebSocketChannelFactory.unregister();
    server.stop(function() {});
  });

  run_next_test();
}
