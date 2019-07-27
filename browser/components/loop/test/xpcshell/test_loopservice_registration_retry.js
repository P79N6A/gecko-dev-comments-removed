


"use strict";

Cu.import("resource://gre/modules/Task.jsm");
Cu.import("resource://services-common/utils.js");





add_test(function test_retry_after_failed_push_reg() {
  mockPushHandler.registrationResult = "404";

  MozLoopService.initialize().then((result) => {
    do_print(result);
    do_throw("should not succeed when loop server registration fails");
  }, Task.async(function*(err) {
    
    
    Assert.equal(err.message, "404", "");

    let regError = MozLoopService.errors.get("initialization");
    Assert.ok(regError, "registration error should be reported");
    Assert.ok(regError.friendlyDetailsButtonCallback, "registration retry callback should exist");

    
    mockPushHandler.registrationResult = null;
    mockPushHandler.registrationPushURL = kEndPointUrl;

    yield regError.friendlyDetailsButtonCallback();
    Assert.strictEqual(MozLoopService.errors.size, 0, "Check that the errors are gone");
    let deferredRegistrations = MozLoopServiceInternal.deferredRegistrations;
    yield deferredRegistrations.get(LOOP_SESSION_TYPE.GUEST).then(() => {
      Assert.ok(true, "The retry of registration succeeded");
    },
    (error) => {
      Assert.ok(false, "The retry of registration should have succeeded");
    });

    run_next_test();
  }));
});

function run_test() {
  setupFakeLoopServer();

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
  });

  Services.prefs.setBoolPref("loop.createdRoom", true);

  do_register_cleanup(function() {
    Services.prefs.clearUserPref("loop.hawk-session-token");
    Services.prefs.clearUserPref("loop.createdRoom");
  });

  run_next_test();
}
