



const PREF_RESTORE_ON_DEMAND = "browser.sessionstore.restore_on_demand";

let stateBackup = ss.getBrowserState();

function test() {
  waitForExplicitFinish();

  Services.prefs.setBoolPref(PREF_RESTORE_ON_DEMAND, true);
  registerCleanupFunction(function () {
    Services.prefs.clearUserPref(PREF_RESTORE_ON_DEMAND);
  });

  let state = { windows: [{ tabs: [
    { entries: [{ url: "http://example.org/#1" }], extData: { "uniq": r() }, pinned: true },
    { entries: [{ url: "http://example.org/#2" }], extData: { "uniq": r() }, pinned: true },
    { entries: [{ url: "http://example.org/#3" }], extData: { "uniq": r() }, pinned: true },
    { entries: [{ url: "http://example.org/#4" }], extData: { "uniq": r() } },
    { entries: [{ url: "http://example.org/#5" }], extData: { "uniq": r() } },
    { entries: [{ url: "http://example.org/#6" }], extData: { "uniq": r() } },
    { entries: [{ url: "http://example.org/#7" }], extData: { "uniq": r() } },
  ], selected: 5 }] };

  let loadCount = 0;
  gProgressListener.setCallback(function (aBrowser, aNeedRestore, aRestoring, aRestored) {
    loadCount++;

    
    

    
    let tab;
    for (let i = 0; i < window.gBrowser.tabs.length; i++) {
      if (!tab && window.gBrowser.tabs[i].linkedBrowser == aBrowser)
        tab = window.gBrowser.tabs[i];
    }

    ok(tab.pinned || gBrowser.selectedTab == tab,
       "load came from pinned or selected tab");

    
    if (loadCount < 4)
      return;

    gProgressListener.unsetCallback();
    executeSoon(function () {
      waitForBrowserState(JSON.parse(stateBackup), finish);
    });
  });

  ss.setBrowserState(JSON.stringify(state));
}
