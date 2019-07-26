



function test() {
  

  waitForExplicitFinish();
  const baseURL = "http://mochi.test:8888/browser/" +
    "browser/components/sessionstore/test/browser_447951_sample.html#";

  
  gPrefService.setIntPref("browser.sessionstore.max_serialize_back", -1);
  gPrefService.setIntPref("browser.sessionstore.max_serialize_forward", -1);
  registerCleanupFunction(function () {
    gPrefService.clearUserPref("browser.sessionstore.max_serialize_back");
    gPrefService.clearUserPref("browser.sessionstore.max_serialize_forward");
  });

  let tab = gBrowser.addTab();
  whenBrowserLoaded(tab.linkedBrowser, function() {
    let tabState = { entries: [] };
    let max_entries = gPrefService.getIntPref("browser.sessionhistory.max_entries");
    for (let i = 0; i < max_entries; i++)
      tabState.entries.push({ url: baseURL + i });

    ss.setTabState(tab, JSON.stringify(tabState));
    whenTabRestored(tab, function() {
      SyncHandlers.get(tab.linkedBrowser).flush();
      tabState = JSON.parse(ss.getTabState(tab));
      is(tabState.entries.length, max_entries, "session history filled to the limit");
      is(tabState.entries[0].url, baseURL + 0, "... but not more");

      
      tab.linkedBrowser.contentDocument.querySelector("a").click();

      function check() {
        SyncHandlers.get(tab.linkedBrowser).flush();
        tabState = JSON.parse(ss.getTabState(tab));
        if (tabState.entries[tabState.entries.length - 1].url != baseURL + "end") {
          
          
          executeSoon(check);
          return;
        }

        is(tab.linkedBrowser.currentURI.spec, baseURL + "end",
           "the new anchor was loaded");
        is(tabState.entries[tabState.entries.length - 1].url, baseURL + "end",
           "... and ignored");
        is(tabState.entries[0].url, baseURL + 1,
           "... and the first item was removed");

        
        gBrowser.removeTab(tab);
        finish();
      }

      check();
    });
  });
}
