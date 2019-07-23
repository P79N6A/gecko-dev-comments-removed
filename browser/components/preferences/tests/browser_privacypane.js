




































function runTestOnPrivacyPrefPane(testFunc) {
  let ww = Cc["@mozilla.org/embedcomp/window-watcher;1"].
           getService(Ci.nsIWindowWatcher);
  let observer = {
    observe: function(aSubject, aTopic, aData) {
      if (aTopic == "domwindowopened") {
        ww.unregisterNotification(this);

        let win = aSubject.QueryInterface(Ci.nsIDOMEventTarget);
        win.addEventListener("load", function() {
          win.removeEventListener("load", arguments.callee, false);
          testFunc(dialog.document.defaultView);

          ww.registerNotification(observer);
          dialog.close();
        }, false);
      } else if (aTopic == "domwindowclosed") {
        ww.unregisterNotification(this);
        testRunner.runNext();
      }
    }
  };
  ww.registerNotification(observer);

  let dialog = openDialog("chrome://browser/content/preferences/preferences.xul", "Preferences",
                          "chrome,titlebar,toolbar,centerscreen,dialog=no", "panePrivacy");
}

function controlChanged(doc, element) {
  let event = doc.createEvent("Events");
  event.initEvent("command", true, true);
  element.dispatchEvent(event);
}

function test_locbar_emptyText(win) {
  let texts = ["none", "bookmarkhistory", "history", "bookmark"];

  let locbarlist = win.document.getElementById("locationBarSuggestion");
  ok(locbarlist, "location bar suggestion menulist should exist");

  for (let level = -1; level <= 2; ++level) {
    locbarlist.value = level;
    controlChanged(win.document, locbarlist);
    is(gURLBar.emptyText, gURLBar.getAttribute(texts[level + 1] + "emptytext"),
      "location bar empty text for for level " + level + " is correctly set");
  }
}

function test_pane_visibility(win) {
  let modes = {
    "remember": "historyRememberPane",
    "dontremember": "historyDontRememberPane",
    "custom": "historyCustomPane"
  };

  let historymode = win.document.getElementById("historyMode");
  ok(historymode, "history mode menulist should exist");
  let historypane = win.document.getElementById("historyPane");
  ok(historypane, "history mode pane should exist");

  for (let mode in modes) {
    historymode.value = mode;
    controlChanged(win.document, historymode);
    is(historypane.selectedPanel, win.document.getElementById(modes[mode]),
      "The correct pane should be selected for the " + mode + " mode");
  }
}

function test_dependent_elements(win) {
  let historymode = win.document.getElementById("historyMode");
  ok(historymode, "history mode menulist should exist");
  let pbautostart = win.document.getElementById("privateBrowsingAutoStart");
  ok(pbautostart, "the private browsing auto-start checkbox should exist");
  let controls = [
    win.document.getElementById("rememberHistoryDays"),
    win.document.getElementById("historyDays"),
    win.document.getElementById("rememberAfter"),
    win.document.getElementById("rememberDownloads"),
    win.document.getElementById("rememberForms"),
    win.document.getElementById("keepUntil"),
    win.document.getElementById("keepCookiesUntil"),
    win.document.getElementById("alwaysClear"),
  ];
  controls.forEach(function(control) {
    ok(control, "the dependent controls should exist");
  });
  let independents = [
    win.document.getElementById("acceptCookies"),
    win.document.getElementById("acceptThirdParty"),
  ];
  independents.forEach(function(control) {
    ok(control, "the independent controls should exist");
  });
  let cookieexceptions = win.document.getElementById("cookieExceptions");
  ok(cookieexceptions, "the cookie exceptions button should exist");
  let keepuntil = win.document.getElementById("keepCookiesUntil");
  ok(keepuntil, "the keep cookies until menulist should exist");
  let alwaysclear = win.document.getElementById("alwaysClear");
  ok(alwaysclear, "the clear data on close checkbox should exist");
  let rememberhistory = win.document.getElementById("rememberHistoryDays");
  ok(rememberhistory, "the remember history checkbox should exist");
  let rememberdownloads = win.document.getElementById("rememberDownloads");
  ok(rememberdownloads, "the remember downloads checkbox should exist");
  let rememberforms = win.document.getElementById("rememberForms");
  ok(rememberforms, "the remember forms checkbox should exist");
  let alwaysclearsettings = win.document.getElementById("clearDataSettings");
  ok(alwaysclearsettings, "the clear data settings button should exist");

  function expect_disabled(disabled) {
    controls.forEach(function(control) {
      is(control.disabled, disabled,
        control.getAttribute("id") + " should " + (disabled ? "" : "not ") + "be disabled");
    });
    is(keepuntil.value, disabled ? 2 : 0,
      "the keep cookies until menulist value should be as expected");
    if (disabled) {
     ok(!alwaysclear.checked,
        "the clear data on close checkbox value should be as expected");
     ok(!rememberhistory.checked,
        "the remember history checkbox value should be as expected");
     ok(!rememberdownloads.checked,
        "the remember downloads checkbox value should be as expected");
     ok(!rememberforms.checked,
        "the remember forms checkbox value should be as expected");
    }
  }
  function check_independents(expected) {
    independents.forEach(function(control) {
      is(control.disabled, expected,
        control.getAttribute("id") + " should " + (expected ? "" : "not ") + "be disabled");
    });
    ok(!cookieexceptions.disabled,
      "the cookie exceptions button should never be disabled");
    ok(alwaysclearsettings.disabled,
      "the clear data settings button should always be disabled");
  }

  
  historymode.value = "remember";
  controlChanged(win.document, historymode);
  expect_disabled(false);
  check_independents(false);

  
  historymode.value = "custom";
  controlChanged(win.document, historymode);
  expect_disabled(false);
  check_independents(false);

  
  historymode.value = "dontremember";
  controlChanged(win.document, historymode);
  expect_disabled(false);
  check_independents(false);

  
  historymode.value = "custom";
  controlChanged(win.document, historymode);
  expect_disabled(true);
  check_independents(false);

  
  pbautostart.checked = false;
  controlChanged(win.document, pbautostart);
  expect_disabled(false);
  check_independents(false);

  
  pbautostart.checked = true;
  controlChanged(win.document, pbautostart);
  expect_disabled(true);
  check_independents(false);
}

function test_dependent_cookie_elements(win) {
  let historymode = win.document.getElementById("historyMode");
  ok(historymode, "history mode menulist should exist");
  let pbautostart = win.document.getElementById("privateBrowsingAutoStart");
  ok(pbautostart, "the private browsing auto-start checkbox should exist");
  let controls = [
    win.document.getElementById("acceptThirdParty"),
    win.document.getElementById("keepUntil"),
    win.document.getElementById("keepCookiesUntil"),
  ];
  controls.forEach(function(control) {
    ok(control, "the dependent cookie controls should exist");
  });
  let acceptcookies = win.document.getElementById("acceptCookies");
  ok(acceptcookies, "the accept cookies checkbox should exist");

  function expect_disabled(disabled) {
    controls.forEach(function(control) {
      is(control.disabled, disabled,
        control.getAttribute("id") + " should " + (disabled ? "" : "not ") + "be disabled");
    });
  }

  historymode.value = "custom";
  controlChanged(win.document, historymode);
  pbautostart.checked = false;
  controlChanged(win.document, pbautostart);
  expect_disabled(false);

  acceptcookies.checked = false;
  controlChanged(win.document, acceptcookies);
  expect_disabled(true);

  
  pbautostart.checked = true;
  controlChanged(win.document, pbautostart);
  expect_disabled(true);

  pbautostart.checked = false;
  controlChanged(win.document, pbautostart);
  expect_disabled(true);

  acceptcookies.checked = true;
  controlChanged(win.document, acceptcookies);
  expect_disabled(false);

  let accessthirdparty = controls.shift();
  pbautostart.checked = true;
  controlChanged(win.document, pbautostart);
  expect_disabled(true);
  ok(!accessthirdparty.disabled, "access third party button should be enabled");

  acceptcookies.checked = false;
  controlChanged(win.document, acceptcookies);
  expect_disabled(true);
  ok(accessthirdparty.disabled, "access third party button should be disabled");

  pbautostart.checked = false;
  controlChanged(win.document, pbautostart);
  expect_disabled(true);
  ok(accessthirdparty.disabled, "access third party button should be disabled");

  acceptcookies.checked = true;
  controlChanged(win.document, acceptcookies);
  expect_disabled(false);
  ok(!accessthirdparty.disabled, "access third party button should be enabled");
}

function test_dependent_clearonclose_elements(win) {
  let historymode = win.document.getElementById("historyMode");
  ok(historymode, "history mode menulist should exist");
  let pbautostart = win.document.getElementById("privateBrowsingAutoStart");
  ok(pbautostart, "the private browsing auto-start checkbox should exist");
  let alwaysclear = win.document.getElementById("alwaysClear");
  ok(alwaysclear, "the clear data on close checkbox should exist");
  let alwaysclearsettings = win.document.getElementById("clearDataSettings");
  ok(alwaysclearsettings, "the clear data settings button should exist");

  function expect_disabled(disabled) {
    is(alwaysclearsettings.disabled, disabled,
      "the clear data settings should " + (disabled ? "" : "not ") + "be disabled");
  }

  historymode.value = "custom";
  controlChanged(win.document, historymode);
  pbautostart.checked = false;
  controlChanged(win.document, pbautostart);
  alwaysclear.checked = false;
  controlChanged(win.document, alwaysclear);
  expect_disabled(true);

  alwaysclear.checked = true;
  controlChanged(win.document, alwaysclear);
  expect_disabled(false);

  pbautostart.checked = true;
  controlChanged(win.document, pbautostart);
  expect_disabled(true);

  pbautostart.checked = false;
  controlChanged(win.document, pbautostart);
  expect_disabled(false);

  alwaysclear.checked = false;
  controlChanged(win.document, alwaysclear);
  expect_disabled(true);
}

function test_dependent_prefs(win) {
  let historymode = win.document.getElementById("historyMode");
  ok(historymode, "history mode menulist should exist");
  let controls = [
    win.document.getElementById("rememberHistoryDays"),
    win.document.getElementById("rememberDownloads"),
    win.document.getElementById("rememberForms"),
    win.document.getElementById("acceptCookies"),
    win.document.getElementById("acceptThirdParty"),
  ];
  controls.forEach(function(control) {
    ok(control, "the micro-management controls should exist");
  });

  function expect_checked(checked) {
    controls.forEach(function(control) {
      is(control.checked, checked,
        control.getAttribute("id") + " should " + (checked ? "not " : "") + "be checked");
    });
  }

  
  historymode.value = "remember";
  controlChanged(win.document, historymode);
  expect_checked(true);

  
  historymode.value = "custom";
  controlChanged(win.document, historymode);
  controls.forEach(function(control) {
    control.checked = false;
    controlChanged(win.document, control);
  });
  expect_checked(false);
  historymode.value = "remember";
  controlChanged(win.document, historymode);
  expect_checked(true);
}

function test_historymode_retention(mode, expect) {
  return function(win) {
    let historymode = win.document.getElementById("historyMode");
    ok(historymode, "history mode menulist should exist");

    if (expect !== undefined) {
      is(historymode.value, expect,
        "history mode is expected to remain " + expect);
    }

    historymode.value = mode;
    controlChanged(win.document, historymode);
  };
}

function test_custom_retention(controlToChange, expect, valueIncrement) {
  return function(win) {
    let historymode = win.document.getElementById("historyMode");
    ok(historymode, "history mode menulist should exist");

    if (expect !== undefined) {
      is(historymode.value, expect,
        "history mode is expected to remain " + expect);
    }

    historymode.value = "custom";
    controlChanged(win.document, historymode);

    controlToChange = win.document.getElementById(controlToChange);
    ok(controlToChange, "the control to change should exist");
    switch (controlToChange.localName) {
    case "checkbox":
      controlToChange.checked = !controlToChange.checked;
      break;
    case "textbox":
      controlToChange.value = parseInt(controlToChange.value) + valueIncrement;
      break;
    case "menulist":
      controlToChange.value = valueIncrement;
      break;
    }
    controlChanged(win.document, controlToChange);
  };
}

function test_locbar_suggestion_retention(mode, expect) {
  return function(win) {
    let locbarsuggest = win.document.getElementById("locationBarSuggestion");
    ok(locbarsuggest, "location bar suggestion menulist should exist");

    if (expect !== undefined) {
      is(locbarsuggest.value, expect,
        "location bar suggestion is expected to remain " + expect);
    }

    locbarsuggest.value = mode;
    controlChanged(win.document, locbarsuggest);
  };
}

function test_privatebrowsing_toggle(win) {
  let historymode = win.document.getElementById("historyMode");
  ok(historymode, "history mode menulist should exist");
  let pbautostart = win.document.getElementById("privateBrowsingAutoStart");
  ok(pbautostart, "the private browsing auto-start checkbox should exist");

  let pbService = Cc["@mozilla.org/privatebrowsing;1"].
                  getService(Ci.nsIPrivateBrowsingService);

  
  historymode.value = "remember";
  controlChanged(win.document, historymode);

  
  historymode.value = "dontremember";
  controlChanged(win.document, historymode);
  ok(pbService.privateBrowsingEnabled, "private browsing should be activated");

  
  historymode.value = "remember";
  controlChanged(win.document, historymode);
  ok(!pbService.privateBrowsingEnabled, "private browsing should be deactivated");

  
  historymode.value = "custom";
  controlChanged(win.document, historymode);
  ok(!pbService.privateBrowsingEnabled, "private browsing should remain deactivated");

  
  pbautostart.checked = true;
  controlChanged(win.document, pbautostart);
  ok(pbService.privateBrowsingEnabled, "private browsing should be activated");

  
  pbautostart.checked = false;
  controlChanged(win.document, pbautostart);
  ok(!pbService.privateBrowsingEnabled, "private browsing should be deactivated");
}

function test_privatebrowsing_ui(win) {
  let historymode = win.document.getElementById("historyMode");
  ok(historymode, "history mode menulist should exist");
  let pbautostart = win.document.getElementById("privateBrowsingAutoStart");
  ok(pbautostart, "the private browsing auto-start checkbox should exist");

  let pbmenuitem = document.getElementById("privateBrowsingItem");
  ok(pbmenuitem, "the private browsing menu item should exist");
  let pbcommand = document.getElementById("Tools:PrivateBrowsing");
  ok(pbcommand, "the private browsing menu item should exist");

  
  historymode.value = "remember";
  controlChanged(win.document, historymode);
  ok(!pbmenuitem.hasAttribute("disabled"),
    "private browsing menu item should not be initially disabled");
  ok(!pbcommand.hasAttribute("disabled"),
    "private browsing command should not be initially disabled");

  
  historymode.value = "dontremember";
  controlChanged(win.document, historymode);
  ok(pbmenuitem.hasAttribute("disabled"),
    "private browsing menu item should be disabled");
  ok(pbcommand.hasAttribute("disabled"),
    "private browsing command should be disabled");

  
  historymode.value = "remember";
  controlChanged(win.document, historymode);
  ok(!pbmenuitem.hasAttribute("disabled"),
    "private browsing menu item should be enabled");
  ok(!pbcommand.hasAttribute("disabled"),
    "private browsing command should be enabled");

  
  historymode.value = "custom";
  controlChanged(win.document, historymode);
  ok(!pbmenuitem.hasAttribute("disabled"),
    "private browsing menu item should remain enabled");
  ok(!pbcommand.hasAttribute("disabled"),
    "private browsing command should remain enabled");

  
  pbautostart.checked = true;
  controlChanged(win.document, pbautostart);
  ok(pbmenuitem.hasAttribute("disabled"),
    "private browsing menu item should be disabled");
  ok(pbcommand.hasAttribute("disabled"),
    "private browsing command should be disabled");

  
  pbautostart.checked = false;
  controlChanged(win.document, pbautostart);
  ok(!pbmenuitem.hasAttribute("disabled"),
    "private browsing menu item should be enabled");
  ok(!pbcommand.hasAttribute("disabled"),
    "private browsing command should be enabled");
}

function enter_private_browsing(win) {
  let pbService = Cc["@mozilla.org/privatebrowsing;1"].
                  getService(Ci.nsIPrivateBrowsingService);
  win.document.getElementById("browser.privatebrowsing.keep_current_session")
              .value = true;
  pbService.privateBrowsingEnabled = true;
}

function reset_preferences(win) {
  let prefs = win.document.getElementsByTagName("preference");
  for (let i = 0; i < prefs.length; ++i)
    if (prefs[i].hasUserValue)
      prefs[i].reset();
}

let testRunner;
function test() {
  let psvc = Cc["@mozilla.org/preferences-service;1"].
             getService(Ci.nsIPrefBranch);
  let instantApplyOrig = psvc.getBoolPref("browser.preferences.instantApply");
  psvc.setBoolPref("browser.preferences.instantApply", true);

  waitForExplicitFinish();

  testRunner = {
    tests: [
      test_locbar_emptyText,
      test_pane_visibility,
      test_dependent_elements,
      test_dependent_cookie_elements,
      test_dependent_clearonclose_elements,
      test_dependent_prefs,
      test_historymode_retention("remember", undefined),
      test_historymode_retention("dontremember", "remember"),
      test_historymode_retention("custom", "dontremember"),
      
      test_historymode_retention("remember", "dontremember"),
      test_historymode_retention("custom", "remember"),
      
      test_historymode_retention("remember", "remember"),
      test_custom_retention("rememberHistoryDays", "remember"),
      test_custom_retention("rememberHistoryDays", "custom"),
      test_custom_retention("historyDays", "remember", 1),
      test_custom_retention("historyDays", "custom", -1),
      test_custom_retention("rememberDownloads", "remember"),
      test_custom_retention("rememberDownloads", "custom"),
      test_custom_retention("rememberForms", "remember"),
      test_custom_retention("rememberForms", "custom"),
      test_custom_retention("acceptCookies", "remember"),
      test_custom_retention("acceptCookies", "custom"),
      test_custom_retention("acceptThirdParty", "remember"),
      test_custom_retention("acceptThirdParty", "custom"),
      test_custom_retention("keepCookiesUntil", "remember", 1),
      test_custom_retention("keepCookiesUntil", "custom", 2),
      test_custom_retention("keepCookiesUntil", "custom", 0),
      test_custom_retention("alwaysClear", "remember"),
      test_custom_retention("alwaysClear", "custom"),
      test_historymode_retention("remember", "remember"),
      test_locbar_suggestion_retention(-1, undefined),
      test_locbar_suggestion_retention(1, -1),
      test_locbar_suggestion_retention(2, 1),
      test_locbar_suggestion_retention(0, 2),
      test_locbar_suggestion_retention(0, 0),
      test_privatebrowsing_toggle,
      enter_private_browsing, 
      test_privatebrowsing_toggle,
      test_privatebrowsing_ui,
      enter_private_browsing, 
      test_privatebrowsing_ui,

      
      reset_preferences
    ],
    counter: 0,
    runNext: function() {
      if (this.counter == this.tests.length) {
        
        psvc.setBoolPref("browser.preferences.instantApply", instantApplyOrig);
        finish();
      } else {
        let self = this;
        setTimeout(function() {
          runTestOnPrivacyPrefPane(self.tests[self.counter++]);
        }, 0);
      }
    }
  };

  testRunner.runNext();
}
