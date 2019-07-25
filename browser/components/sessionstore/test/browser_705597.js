


let tabState = {
  entries: [{url: "about:home", children: [{url: "about:mozilla"}]}]
};

function test() {
  waitForExplicitFinish();
  requestLongerTimeout(2);

  Services.prefs.setIntPref("browser.sessionstore.interval", 4000);
  registerCleanupFunction(function () {
    Services.prefs.clearUserPref("browser.sessionstore.interval");
  });

  let tab = gBrowser.addTab("about:blank");

  let browser = tab.linkedBrowser;

  whenBrowserLoaded(browser, function () {
    ss.setTabState(tab, JSON.stringify(tabState));

    let sessionHistory = browser.sessionHistory;
    let entry = sessionHistory.getEntryAtIndex(0, false);

    whenChildCount(entry, 1, function () {
      whenChildCount(entry, 2, function () {
        whenBrowserLoaded(browser, function () {
          let {entries} = JSON.parse(ss.getTabState(tab));
          is(entries.length, 1, "tab has one history entry");
          ok(!entries[0].children, "history entry has no subframes");

          
          let blankState = { windows: [{ tabs: [{ entries: [{ url: "about:blank" }] }]}]};
          waitForBrowserState(blankState, finish);
        });

        
        browser.reload();
      });

      
      let doc = browser.contentDocument;
      let iframe = doc.createElement("iframe");
      doc.body.appendChild(iframe);
      iframe.setAttribute("src", "about:mozilla");
    });
  });
}

function whenBrowserLoaded(aBrowser, aCallback) {
  aBrowser.addEventListener("load", function onLoad() {
    aBrowser.removeEventListener("load", onLoad, true);
    executeSoon(aCallback);
  }, true);
}

function whenChildCount(aEntry, aChildCount, aCallback) {
  if (aEntry.childCount == aChildCount)
    aCallback();
  else
    setTimeout(function () whenChildCount(aEntry, aChildCount, aCallback), 100);
}
