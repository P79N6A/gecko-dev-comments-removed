







"use strict";

Components.utils.import("resource://gre/modules/Promise.jsm", this);

add_task(loadLoopPanel);

add_task(function* test_mozLoop_doNotDisturb() {
  registerCleanupFunction(function () {
    Services.prefs.clearUserPref("loop.do_not_disturb");
  });

  Assert.ok(gMozLoopAPI, "mozLoop should exist");

  
  Services.prefs.setBoolPref("loop.do_not_disturb", true);
  Assert.equal(gMozLoopAPI.doNotDisturb, true,
               "Do not disturb should be true");

  
  gMozLoopAPI.doNotDisturb = false;
  Assert.equal(Services.prefs.getBoolPref("loop.do_not_disturb"), false,
               "Do not disturb should be false");
});
