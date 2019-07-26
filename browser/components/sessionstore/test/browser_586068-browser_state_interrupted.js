



const PREF_RESTORE_ON_DEMAND = "browser.sessionstore.restore_on_demand";

function test() {
  TestRunner.run();
}

function runTests() {
  Services.prefs.setBoolPref(PREF_RESTORE_ON_DEMAND, false);
  registerCleanupFunction(function () {
    Services.prefs.clearUserPref(PREF_RESTORE_ON_DEMAND);
  });

  
  
  let state1 = { windows: [
    {
      tabs: [
        { entries: [{ url: "http://example.org#1" }], extData: { "uniq": r() } },
        { entries: [{ url: "http://example.org#2" }], extData: { "uniq": r() } },
        { entries: [{ url: "http://example.org#3" }], extData: { "uniq": r() } },
        { entries: [{ url: "http://example.org#4" }], extData: { "uniq": r() } }
      ],
      selected: 1
    },
    {
      tabs: [
        { entries: [{ url: "http://example.com#1" }], extData: { "uniq": r() } },
        { entries: [{ url: "http://example.com#2" }], extData: { "uniq": r() } },
        { entries: [{ url: "http://example.com#3" }], extData: { "uniq": r() } },
        { entries: [{ url: "http://example.com#4" }], extData: { "uniq": r() } },
      ],
      selected: 3
    }
  ] };
  let state2 = { windows: [
    {
      tabs: [
        { entries: [{ url: "http://example.org#5" }], extData: { "uniq": r() } },
        { entries: [{ url: "http://example.org#6" }], extData: { "uniq": r() } },
        { entries: [{ url: "http://example.org#7" }], extData: { "uniq": r() } },
        { entries: [{ url: "http://example.org#8" }], extData: { "uniq": r() } }
      ],
      selected: 3
    },
    {
      tabs: [
        { entries: [{ url: "http://example.com#5" }], extData: { "uniq": r() } },
        { entries: [{ url: "http://example.com#6" }], extData: { "uniq": r() } },
        { entries: [{ url: "http://example.com#7" }], extData: { "uniq": r() } },
        { entries: [{ url: "http://example.com#8" }], extData: { "uniq": r() } },
      ],
      selected: 1
    }
  ] };

  
  let interruptedAfter = 0;
  let loadedWindow1 = false;
  let loadedWindow2 = false;
  let numTabs = state2.windows[0].tabs.length + state2.windows[1].tabs.length;

  let loadCount = 0;
  gProgressListener.setCallback(function (aBrowser, aNeedRestore, aRestoring, aRestored) {
    loadCount++;

    if (aBrowser.currentURI.spec == state1.windows[0].tabs[2].entries[0].url)
      loadedWindow1 = true;
    if (aBrowser.currentURI.spec == state1.windows[1].tabs[0].entries[0].url)
      loadedWindow2 = true;

    if (!interruptedAfter && loadedWindow1 && loadedWindow2) {
      interruptedAfter = loadCount;
      ss.setBrowserState(JSON.stringify(state2));
      return;
    }

    if (loadCount < numTabs + interruptedAfter)
      return;

    
    
    is(loadCount, numTabs + interruptedAfter, "all tabs were restored");
    is(aNeedRestore, 0, "there are no tabs left needing restore");

    
    
    gProgressListener.unsetCallback();
    executeSoon(next);
  });

  
  Services.ww.registerNotification(function observer(aSubject, aTopic, aData) {
    if (aTopic == "domwindowopened") {
      let win = aSubject.QueryInterface(Ci.nsIDOMWindow);
      win.addEventListener("load", function onLoad() {
        win.removeEventListener("load", onLoad);
        Services.ww.unregisterNotification(observer);
        win.gBrowser.addTabsProgressListener(gProgressListener);
      });
    }
  });

  yield ss.setBrowserState(JSON.stringify(state1));
}
