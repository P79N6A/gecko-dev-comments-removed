


let tabState = {
  entries: [{url: "about:robots", children: [{url: "about:mozilla"}]}]
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

  waitForTabState(tab, tabState, function() {

    let sessionHistory = browser.sessionHistory;
    let entry = sessionHistory.getEntryAtIndex(0, false);
    entry.QueryInterface(Ci.nsISHContainer);

    whenChildCount(entry, 1, function () {
      whenChildCount(entry, 2, function () {
        whenBrowserLoaded(browser, function () {
          let sessionHistory = browser.sessionHistory;
          let entry = sessionHistory.getEntryAtIndex(0, false);

          whenChildCount(entry, 0, function () {
            
            let blankState = { windows: [{ tabs: [{ entries: [{ url: "about:blank" }] }]}]};
            waitForBrowserState(blankState, finish);
          });
        });

        
        browser.reload();
      });

      
      let doc = browser.contentDocument;
      let iframe = doc.createElement("iframe");
      doc.body.appendChild(iframe);
      iframe.setAttribute("src", "about:mozilla");
    });
  });

  
  
  ok(true, "Each test requires at least one pass, fail or todo so here is a pass.");
}

function whenChildCount(aEntry, aChildCount, aCallback) {
  if (aEntry.childCount == aChildCount)
    aCallback();
  else
    setTimeout(function () whenChildCount(aEntry, aChildCount, aCallback), 100);
}
