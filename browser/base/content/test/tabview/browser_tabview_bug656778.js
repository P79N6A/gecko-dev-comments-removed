


function test() {
  waitForExplicitFinish();

  registerCleanupFunction(function () {
    Services.prefs.clearUserPref(TabView.PREF_FIRST_RUN);
    Services.prefs.clearUserPref(TabView.PREF_STARTUP_PAGE);
    Services.prefs.clearUserPref(TabView.PREF_RESTORE_ENABLED_ONCE);
  });

  let assertBoolPref = function (pref, value) {
    is(Services.prefs.getBoolPref(pref), value, pref + " is " + value);
  };

  let assertIntPref = function (pref, value) {
    is(Services.prefs.getIntPref(pref), value, pref + " is " + value);
  };

  let setPreferences = function (startupPage, firstRun, enabledOnce) {
    Services.prefs.setIntPref(TabView.PREF_STARTUP_PAGE, startupPage);
    Services.prefs.setBoolPref(TabView.PREF_FIRST_RUN, firstRun);
    Services.prefs.setBoolPref(TabView.PREF_RESTORE_ENABLED_ONCE, enabledOnce);
  };

  let assertPreferences = function (startupPage, firstRun, enabledOnce) {
    assertIntPref(TabView.PREF_STARTUP_PAGE, startupPage);
    assertBoolPref(TabView.PREF_FIRST_RUN, firstRun);
    assertBoolPref(TabView.PREF_RESTORE_ENABLED_ONCE, enabledOnce);
  };

  let next = function () {
    if (tests.length == 0) {
      waitForFocus(finish);
      return;
    }

    let test = tests.shift();
    info("running " + test.name + "...");
    test();
  };

  
  
  
  
  
  
  
  let test1 = function test1() {
    setPreferences(1, true, false);

    newWindowWithTabView(function (win) {
      assertPreferences(3, true, true);

      win.close();
      next();
    });
  };

  
  
  
  
  
  
  
  
  
  let test2 = function test2() {
    setPreferences(1, false, false);

    newWindowWithTabView(function (win) {
      assertPreferences(1, false, false);

      win.TabView.firstUseExperienced = true;

      assertPreferences(3, true, true);

      win.close();
      next();
    });
  };

  
  
  
  
  
  
  
  let test3 = function test3() {
    setPreferences(3, true, false);

    newWindowWithTabView(function (win) {
      assertPreferences(3, true, true);

      win.close();
      next();
    });
  };

  
  
  
  
  
  
  let test4 = function test4() {
    setPreferences(3, true, true);

    newWindowWithTabView(function (win) {
      assertPreferences(3, true, true);

      win.close();
      next();
    });
  };

  
  
  
  
  
  
  
  let test5 = function test5() {
    setPreferences(1, true, true);

    newWindowWithTabView(function (win) {
      assertPreferences(1, true, true);

      win.close();
      next();
    });
  };

  let tests = [test1, test2, test3, test4, test5];
  next();
}
