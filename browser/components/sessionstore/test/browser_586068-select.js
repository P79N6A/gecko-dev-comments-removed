



const PREF_RESTORE_ON_DEMAND = "browser.sessionstore.restore_on_demand";

function test() {
  TestRunner.run();
}

function runTests() {
  Services.prefs.setBoolPref(PREF_RESTORE_ON_DEMAND, true);
  registerCleanupFunction(function () {
    Services.prefs.clearUserPref(PREF_RESTORE_ON_DEMAND);
  });

  let state = { windows: [{ tabs: [
    { entries: [{ url: "http://example.org" }], extData: { "uniq": r() } },
    { entries: [{ url: "http://example.org" }], extData: { "uniq": r() } },
    { entries: [{ url: "http://example.org" }], extData: { "uniq": r() } },
    { entries: [{ url: "http://example.org" }], extData: { "uniq": r() } },
    { entries: [{ url: "http://example.org" }], extData: { "uniq": r() } },
    { entries: [{ url: "http://example.org" }], extData: { "uniq": r() } }
  ], selected: 1 }] };

  let expectedCounts = [
    [5, 1, 0],
    [4, 1, 1],
    [3, 1, 2],
    [2, 1, 3],
    [1, 1, 4],
    [0, 1, 5]
  ];
  let tabOrder = [0, 5, 1, 4, 3, 2];

  let loadCount = 0;
  gProgressListener.setCallback(function (aBrowser, aNeedRestore, aRestoring, aRestored) {
    loadCount++;
    let expected = expectedCounts[loadCount - 1];

    is(aNeedRestore, expected[0], "load " + loadCount + " - # tabs that need to be restored");
    is(aRestoring, expected[1], "load " + loadCount + " - # tabs that are restoring");
    is(aRestored, expected[2], "load " + loadCount + " - # tabs that has been restored");

    if (loadCount < state.windows[0].tabs.length) {
      
      let expectedData = state.windows[0].tabs[tabOrder[loadCount - 1]].extData.uniq;
      let tab;
      for (let i = 0; i < window.gBrowser.tabs.length; i++) {
        if (!tab && window.gBrowser.tabs[i].linkedBrowser == aBrowser)
          tab = window.gBrowser.tabs[i];
      }

      is(ss.getTabValue(tab, "uniq"), expectedData,
        "load " + loadCount + " - correct tab was restored");

      
      window.gBrowser.selectTabAtIndex(tabOrder[loadCount]);
    } else {
      gProgressListener.unsetCallback();
      executeSoon(next);
    }
  });

  yield ss.setBrowserState(JSON.stringify(state));
}
