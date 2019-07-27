



const PREF_RESTORE_ON_DEMAND = "browser.sessionstore.restore_on_demand";

add_task(function* test() {
  Services.prefs.setBoolPref(PREF_RESTORE_ON_DEMAND, false);
  registerCleanupFunction(function () {
    Services.prefs.clearUserPref(PREF_RESTORE_ON_DEMAND);
  });

  
  
  let state = { windows: [
    {
      tabs: [
        { entries: [{ url: "http://example.org#0" }], extData: { "uniq": r() } }
      ],
      selected: 1
    },
    {
      tabs: [
        { entries: [{ url: "http://example.com#1" }], extData: { "uniq": r() } },
        { entries: [{ url: "http://example.com#2" }], extData: { "uniq": r() } },
        { entries: [{ url: "http://example.com#3" }], extData: { "uniq": r() } },
        { entries: [{ url: "http://example.com#4" }], extData: { "uniq": r() } },
        { entries: [{ url: "http://example.com#5" }], extData: { "uniq": r() } },
        { entries: [{ url: "http://example.com#6" }], extData: { "uniq": r() } }
      ],
      selected: 4
    }
  ] };
  let numTabs = state.windows[0].tabs.length + state.windows[1].tabs.length;

  let loadCount = 0;
  let promiseRestoringTabs = new Promise(resolve => {
    gProgressListener.setCallback(function (aBrowser, aNeedRestore, aRestoring, aRestored) {
      if (++loadCount == numTabs) {
        
        
        is(loadCount, numTabs, "all tabs were restored");
        is(aNeedRestore, 0, "there are no tabs left needing restore");

        gProgressListener.unsetCallback();
        resolve();
      }
    });
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

  let backupState = ss.getBrowserState();
  ss.setBrowserState(JSON.stringify(state));
  yield promiseRestoringTabs;

  
  yield promiseAllButPrimaryWindowClosed();
  yield promiseBrowserState(backupState);
});
