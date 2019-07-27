


Cu.import("resource://services-common/utils.js");











add_test(function test_register_offline() {
  mockPushHandler.registrationResult = "offline";

  
  Services.io.offline = true;

  MozLoopService.register(mockPushHandler).then(() => {
    do_throw("should not succeed when offline");
  }, err => {
    Assert.equal(err, "offline", "should reject with 'offline' when offline");
    Services.io.offline = false;
    run_next_test();
  });
});





add_test(function test_register_websocket_success_loop_server_fail() {
  mockPushHandler.registrationResult = null;

  MozLoopService.register(mockPushHandler).then(() => {
    do_throw("should not succeed when loop server registration fails");
  }, err => {
    
    
    Assert.equal(err, 404, "Expected no errors in websocket registration");

    run_next_test();
  });
});





add_test(function test_register_success() {
  mockPushHandler.registrationPushURL = kEndPointUrl;

  loopServer.registerPathHandler("/registration", (request, response) => {
    let body = CommonUtils.readBytesFromInputStream(request.bodyInputStream);
    let data = JSON.parse(body);
    Assert.equal(data.simplePushURL, kEndPointUrl,
                 "Should send correct push url");

    response.setStatusLine(null, 200, "OK");
    response.processAsync();
    response.finish();
  });
  MozLoopService.register(mockPushHandler).then(() => {
    run_next_test();
  }, err => {
    do_throw("shouldn't error on a successful request");
  });
});

function run_test()
{
  setupFakeLoopServer();

  do_register_cleanup(function() {
    Services.prefs.clearUserPref("loop.hawk-session-token");
  });

  run_next_test();
}
