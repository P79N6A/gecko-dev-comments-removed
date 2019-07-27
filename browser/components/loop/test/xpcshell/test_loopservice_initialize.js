


"use strict";

let startTimerCalled = false;





add_task(function* test_initialize_no_expiry() {
  startTimerCalled = false;

  let initializedPromise = yield MozLoopService.initialize();
  Assert.equal(initializedPromise, "registration not needed",
               "Promise should be fulfilled");
  Assert.equal(startTimerCalled, false,
    "should not register when no expiry time is set");
});





add_task(function test_initialize_no_guest_rooms() {
  Services.prefs.setBoolPref("loop.createdRoom", false);
  startTimerCalled = false;

  MozLoopService.initialize();

  Assert.equal(startTimerCalled, false,
    "should not register when no guest rooms have been created");
});





add_task(function test_initialize_with_guest_rooms() {
  Services.prefs.setBoolPref("loop.createdRoom", true);
  startTimerCalled = false;

  MozLoopService.initialize();

  Assert.equal(startTimerCalled, true,
    "should start the timer when guest rooms have been created");
});

function run_test() {
  setupFakeLoopServer();

  
  
  MozLoopService.initializeTimerFunc = function() {
    startTimerCalled = true;
  };

  do_register_cleanup(function() {
    Services.prefs.clearUserPref("loop.createdRoom");
  });

  run_next_test();
}
