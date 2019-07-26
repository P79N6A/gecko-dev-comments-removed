



function test() {
  

  waitForExplicitFinish();
  const baseURL = "http://mochi.test:8888/browser/" +
    "browser/components/sessionstore/test/browser_447951_sample.html#";

  let tab = gBrowser.addTab();
  whenBrowserLoaded(tab.linkedBrowser, function() {
    let tabState = { entries: [] };
    let max_entries = gPrefService.getIntPref("browser.sessionhistory.max_entries");
    for (let i = 0; i < max_entries; i++)
      tabState.entries.push({ url: baseURL + i });

    ss.setTabState(tab, JSON.stringify(tabState));
    whenTabRestored(tab, function() {
      tabState = JSON.parse(ss.getTabState(tab));
      is(tabState.entries.length, max_entries, "session history filled to the limit");
      is(tabState.entries[0].url, baseURL + 0, "... but not more");

      
      let doc = tab.linkedBrowser.contentDocument;
      let event = doc.createEvent("MouseEvents");
      event.initMouseEvent("click", true, true, doc.defaultView, 1,
                           0, 0, 0, 0, false, false, false, false, 0, null);
      doc.querySelector("a").dispatchEvent(event);

      function check() {
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
