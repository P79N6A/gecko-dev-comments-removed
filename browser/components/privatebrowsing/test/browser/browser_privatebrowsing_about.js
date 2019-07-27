






function* openAboutPrivateBrowsing() {
  let win = yield BrowserTestUtils.openNewBrowserWindow({ private: true });
  let tab = win.gBrowser.selectedBrowser;
  tab.loadURI("about:privatebrowsing");
  yield BrowserTestUtils.browserLoaded(tab);
  return { win, tab };
}




function* testLinkOpensTab({ win, tab, elementId, expectedUrl }) {
  let newTabPromise = BrowserTestUtils.waitForNewTab(win.gBrowser, expectedUrl);
  yield ContentTask.spawn(tab, { elementId }, function* ({ elementId }) {
    content.document.getElementById(elementId).click();
  });
  let newTab = yield newTabPromise;
  ok(true, `Clicking ${elementId} opened ${expectedUrl} in a new tab.`);
  yield BrowserTestUtils.removeTab(newTab);
}






function* testLinkOpensUrl({ win, tab, elementId, expectedUrl }) {
  let loadedPromise = BrowserTestUtils.browserLoaded(tab);
  yield ContentTask.spawn(tab, { elementId }, function* ({ elementId }) {
    content.document.getElementById(elementId).click();
  });
  yield loadedPromise;
  is(tab.currentURI.spec, expectedUrl,
     `Clicking ${elementId} opened ${expectedUrl} in the same tab.`);
}




add_task(function* test_classicActions() {
  
  Services.prefs.setBoolPref("privacy.trackingprotection.ui.enabled", false);
  Services.prefs.setCharPref("app.support.baseURL", "https://example.com/");
  registerCleanupFunction(function () {
    Services.prefs.clearUserPref("app.support.baseURL");
    Services.prefs.clearUserPref("privacy.trackingprotection.ui.enabled");
  });

  let { win, tab } = yield openAboutPrivateBrowsing();

  yield testLinkOpensTab({ win, tab,
    elementId: "learnMore",
    expectedUrl: "https://example.com/private-browsing",
  });

  yield BrowserTestUtils.closeWindow(win);
});




add_task(function* test_tourActions() {
  
  Services.prefs.setBoolPref("privacy.trackingprotection.ui.enabled", true);
  Services.prefs.setCharPref("app.support.baseURL", "https://example.com/");
  Services.prefs.setCharPref("privacy.trackingprotection.introURL",
                             "https://example.com/tour");
  registerCleanupFunction(function () {
    Services.prefs.clearUserPref("privacy.trackingprotection.introURL");
    Services.prefs.clearUserPref("app.support.baseURL");
    Services.prefs.clearUserPref("privacy.trackingprotection.ui.enabled");
  });

  let { win, tab } = yield openAboutPrivateBrowsing();

  yield testLinkOpensTab({ win, tab,
    elementId: "showPreferences",
    expectedUrl: "about:preferences#privacy",
  });

  yield testLinkOpensTab({ win, tab,
    elementId: "tourLearnMore",
    expectedUrl: "https://example.com/private-browsing",
  });

  yield testLinkOpensUrl({ win, tab,
    elementId: "startTour",
    expectedUrl: "https://example.com/tour",
  });

  yield BrowserTestUtils.closeWindow(win);
});





add_task(function* test_enableTrackingProtection() {
  
  Services.prefs.setBoolPref("privacy.trackingprotection.ui.enabled", true);
  Services.prefs.setBoolPref("privacy.trackingprotection.pbmode.enabled",
                             false);
  registerCleanupFunction(function () {
    Services.prefs.clearUserPref("privacy.trackingprotection.pbmode.enabled");
    Services.prefs.clearUserPref("privacy.trackingprotection.ui.enabled");
  });

  let { win, tab } = yield openAboutPrivateBrowsing();

  
  let prefBranch =
      Services.prefs.getBranch("privacy.trackingprotection.pbmode.");
  let promisePrefChanged = new Promise(resolve => {
    let prefObserver = {
      QueryInterface: XPCOMUtils.generateQI([Ci.nsIObserver]),
      observe: function () {
        prefBranch.removeObserver("enabled", prefObserver);
        resolve();
      },
    };
    prefBranch.addObserver("enabled", prefObserver, false);
  });

  yield ContentTask.spawn(tab, {}, function* () {
    content.document.getElementById("enableTrackingProtection").click();
  });

  yield promisePrefChanged;
  ok(prefBranch.getBoolPref("enabled"), "Tracking Protection is enabled.");

  yield BrowserTestUtils.closeWindow(win);
});
