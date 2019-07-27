


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
  setupFakeLoopServer();

  
  
  MozLoopService.initializeTimerFunc = function() {
    startTimerCalled = true;
  };

  run_next_test();
}
