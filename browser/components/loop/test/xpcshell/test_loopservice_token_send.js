


add_test(function test_registration_uses_hawk_session_token() {
  Services.prefs.setCharPref("loop.hawk-session-token",
    "1bad3e44b12f77a88fe09f016f6a37c42e40f974bc7a8b432bb0d2f0e37e1750");

  loopServer.registerPathHandler("/registration", (request, response) => {
    
    
    
    
    
    let header = request.getHeader("Authorization");

    Assert.notEqual(header.contains("Hawk id="), -1, "Should contain a hawk id");
    Assert.notEqual(header.contains("ts="), -1, "Should contain a timestamp");
    Assert.notEqual(header.contains("nonce="), -1, "Should contain a nonce");
    Assert.notEqual(header.contains("hash="), -1, "Should contain a hash");
    Assert.notEqual(header.contains("mac="), -1, "Should contain a mac");

    response.setStatusLine(null, 200, "OK");
    response.processAsync();
    response.finish();
  });

  MozLoopService.promiseRegisteredWithServers().then(() => {
    run_next_test();
  }, err => {
    do_throw("shouldn't error on a succesful request");
  });
});


function run_test() {
  setupFakeLoopServer();

  mockPushHandler.registrationPushURL = kEndPointUrl;

  do_register_cleanup(function() {
    Services.prefs.clearUserPref("loop.hawk-session-token");
  });

  run_next_test();
}
