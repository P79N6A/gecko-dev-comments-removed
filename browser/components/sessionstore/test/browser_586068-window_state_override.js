



const PREF_RESTORE_ON_DEMAND = "browser.sessionstore.restore_on_demand";

function test() {
  TestRunner.run();
}

function runTests() {
  Services.prefs.setBoolPref(PREF_RESTORE_ON_DEMAND, false);
  registerCleanupFunction(function () {
    Services.prefs.clearUserPref(PREF_RESTORE_ON_DEMAND);
  });

  
  
  let state1 = { windows: [{ tabs: [
    { entries: [{ url: "http://example.com#1" }] },
    { entries: [{ url: "http://example.com#2" }] },
    { entries: [{ url: "http://example.com#3" }] },
    { entries: [{ url: "http://example.com#4" }] },
    { entries: [{ url: "http://example.com#5" }] },
  ] }] };
  let state2 = { windows: [{ tabs: [
    { entries: [{ url: "http://example.org#1" }] },
    { entries: [{ url: "http://example.org#2" }] },
    { entries: [{ url: "http://example.org#3" }] },
    { entries: [{ url: "http://example.org#4" }] },
    { entries: [{ url: "http://example.org#5" }] }
  ] }] };
  let numTabs = 2 + state2.windows[0].tabs.length;

  let loadCount = 0;
  gProgressListener.setCallback(function (aBrowser, aNeedRestore, aRestoring, aRestored) {
    
    if (++loadCount == 2) {
      ss.setWindowState(window, JSON.stringify(state2), true);
    }

    if (loadCount < numTabs) {
      return;
    }

    
    
    is(loadCount, numTabs, "all tabs were restored");
    
    
    
    is(window.__SS_tabsToRestore, 1, "window doesn't think there are more tabs to restore");
    is(aNeedRestore, 0, "there are no tabs left needing restore");

    gProgressListener.unsetCallback();
    executeSoon(next);
  });

  yield ss.setWindowState(window, JSON.stringify(state1), true);
}
