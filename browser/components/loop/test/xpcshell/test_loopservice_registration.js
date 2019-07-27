


"use strict";

Cu.import("resource://services-common/utils.js");












add_test(function test_register_websocket_success_loop_server_fail() {
  mockPushHandler.registrationResult = "404";

  MozLoopService.promiseRegisteredWithServers().then(() => {
    do_throw("should not succeed when loop server registration fails");
  }, (err) => {
    
    
    Assert.equal(err.message, "404", "Expected no errors in websocket registration");

    run_next_test();
  });
});






add_test(function test_register_success() {
  mockPushHandler.registrationPushURL = kEndPointUrl;
  mockPushHandler.registrationResult = null;

  loopServer.registerPathHandler("/registration", (request, response) => {
    let body = CommonUtils.readBytesFromInputStream(request.bodyInputStream);
    let data = JSON.parse(body);
    if (data.simplePushURLs.calls) {
      Assert.equal(data.simplePushURLs.calls, kEndPointUrl,
                   "Should send correct calls push url");
    }
    if (data.simplePushURLs.rooms) {
      Assert.equal(data.simplePushURLs.rooms, kEndPointUrl,
                   "Should send correct rooms push url");
    }

    response.setStatusLine(null, 200, "OK");
    response.processAsync();
    response.finish();
  });
  MozLoopService.promiseRegisteredWithServers().then(() => {
    run_next_test();
  }, err => {
    do_throw("shouldn't error on a successful request");
  });
});

function run_test() {
  setupFakeLoopServer();

  do_register_cleanup(function() {
    Services.prefs.clearUserPref("loop.hawk-session-token");
  });

  run_next_test();
}
