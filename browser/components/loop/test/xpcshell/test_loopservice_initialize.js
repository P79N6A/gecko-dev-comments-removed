


XPCOMUtils.defineLazyModuleGetter(this, "MozLoopService",
                                  "resource:///modules/loop/MozLoopService.jsm");

var server;

const kServerPushUrl = "http://localhost:3456";

var startTimerCalled = false;





add_task(function test_initialize_no_expiry() {
  startTimerCalled = false;

  MozLoopService.initialize();

  Assert.equal(startTimerCalled, false,
    "should not register when no expiry time is set");
});





add_task(function test_initialize_expiry_past() {
  
  var nowSeconds = Date.now() / 1000;
  Services.prefs.setIntPref("loop.urlsExpiryTimeSeconds", nowSeconds - 2);
  startTimerCalled = false;

  MozLoopService.initialize();

  Assert.equal(startTimerCalled, false,
    "should not register when expiry time is in past");
});





add_task(function test_initialize_starts_timer() {
  
  var nowSeconds = Date.now() / 1000;
  Services.prefs.setIntPref("loop.urlsExpiryTimeSeconds", nowSeconds + 60);
  startTimerCalled = false;

  MozLoopService.initialize();

  Assert.equal(startTimerCalled, true,
    "should start the timer when expiry time is in the future");
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

  
  
  MozLoopService._startInitializeTimer = function() {
    startTimerCalled = true;
  };

  do_register_cleanup(function() {
    gMockWebSocketChannelFactory.unregister();
    server.stop(function() {});
  });

  run_next_test();
}
