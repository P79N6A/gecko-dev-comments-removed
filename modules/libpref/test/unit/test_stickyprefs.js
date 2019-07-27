


Components.utils.import("resource://gre/modules/XPCOMUtils.jsm");
Components.utils.import("resource://gre/modules/Services.jsm");

const ps = Services.prefs;






do_get_profile();
do_register_cleanup(saveAndReload);


function resetAndLoad(filenames) {
  ps.resetPrefs();
  for (let filename of filenames) {
    ps.readUserPrefs(do_get_file(filename));
  }
}





function saveAndReload() {
  let file = do_get_profile();
  file.append("prefs.js");
  ps.savePrefFile(file);

  
  ps.resetPrefs();
  ps.readUserPrefs(file);
}

function run_test() {
  run_next_test();
}


add_test(function notWrittenWhenUnchanged() {
  resetAndLoad(["data/testPrefSticky.js"]);
  Assert.strictEqual(ps.getBoolPref("testPref.unsticky.bool"), true);
  Assert.strictEqual(ps.getBoolPref("testPref.sticky.bool"), false);

  
  saveAndReload();
  
  try {
    ps.getBoolPref("testPref.sticky.bool");
    Assert.ok(false, "expected failure reading this pref");
  } catch (ex) {
    Assert.ok(ex, "exception reading regular pref");
  }
  run_next_test();
});



add_test(function writtenOnceLoadedWithoutChange() {
  
  
  
  resetAndLoad(["data/testPrefSticky.js", "data/testPrefStickyUser.js"]);
  
  saveAndReload();
  Assert.strictEqual(ps.getBoolPref("testPref.sticky.bool"), false,
                     "user_pref was written with default value");
  run_next_test();
});


add_test(function writtenOnceLoadedWithChangeNonDefault() {
  
  
  resetAndLoad(["data/testPrefSticky.js", "data/testPrefStickyUser.js"]);
  
  ps.setBoolPref("testPref.sticky.bool", false);
  saveAndReload();
  Assert.strictEqual(ps.getBoolPref("testPref.sticky.bool"), false,
                     "user_pref was written with custom value");
  run_next_test();
});


add_test(function writtenOnceLoadedWithChangeNonDefault() {
  
  
  resetAndLoad(["data/testPrefSticky.js", "data/testPrefStickyUser.js"]);
  
  ps.setBoolPref("testPref.sticky.bool", true);
  saveAndReload();
  Assert.strictEqual(ps.getBoolPref("testPref.sticky.bool"), true,
                     "user_pref was written with custom value");
  run_next_test();
});






add_test(function hasUserValue() {
  
  resetAndLoad(["data/testPrefSticky.js"]);
  Assert.strictEqual(ps.getBoolPref("testPref.sticky.bool"), false);
  Assert.ok(!ps.prefHasUserValue("testPref.sticky.bool"),
            "should not initially reflect a user value");

  ps.setBoolPref("testPref.sticky.bool", false);
  Assert.ok(ps.prefHasUserValue("testPref.sticky.bool"),
            "should reflect a user value after set to default");

  ps.setBoolPref("testPref.sticky.bool", true);
  Assert.ok(ps.prefHasUserValue("testPref.sticky.bool"),
            "should reflect a user value after change to non-default");

  ps.clearUserPref("testPref.sticky.bool");
  Assert.ok(!ps.prefHasUserValue("testPref.sticky.bool"),
            "should reset to no user value");
  ps.setBoolPref("testPref.sticky.bool", false, "expected default");

  
  resetAndLoad(["data/testPrefSticky.js", "data/testPrefStickyUser.js"]);
  Assert.strictEqual(ps.getBoolPref("testPref.sticky.bool"), false);
  Assert.ok(ps.prefHasUserValue("testPref.sticky.bool"),
            "should have a user value when loaded value is the default");
  run_next_test();
});


add_test(function clearUserPref() {
  
  
  resetAndLoad(["data/testPrefSticky.js", "data/testPrefStickyUser.js"]);
  ps.clearUserPref("testPref.sticky.bool");

  
  saveAndReload();
  try {
    ps.getBoolPref("testPref.sticky.bool");
    Assert.ok(false, "expected failure reading this pref");
  } catch (ex) {
    Assert.ok(ex, "pref doesn't have a sticky value");
  }
  run_next_test();
});






add_test(function observerFires() {
  
  resetAndLoad(["data/testPrefSticky.js"]);

  function observe(subject, topic, data) {
    Assert.equal(data, "testPref.sticky.bool");
    ps.removeObserver("testPref.sticky.bool", observe);
    run_next_test();
  }
  ps.addObserver("testPref.sticky.bool", observe, false);

  ps.setBoolPref("testPref.sticky.bool", ps.getBoolPref("testPref.sticky.bool"));
  
});
