


function expiryTimePref() {
  return Services.prefs.getIntPref("loop.urlsExpiryTimeSeconds");
}

function run_test()
{
  setupFakeLoopServer();

  Services.prefs.setIntPref("loop.urlsExpiryTimeSeconds", 0);

  MozLoopService.noteCallUrlExpiry(1000);

  Assert.equal(expiryTimePref(), 1000, "should be equal to set value");

  MozLoopService.noteCallUrlExpiry(900);

  Assert.equal(expiryTimePref(), 1000, "should remain the same value");

  MozLoopService.noteCallUrlExpiry(1500);

  Assert.equal(expiryTimePref(), 1500, "should be the increased value");
}
