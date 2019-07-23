



































function browserWindowsCount() {
  let count = 0;
  let e = Cc["@mozilla.org/appshell/window-mediator;1"]
            .getService(Ci.nsIWindowMediator)
            .getEnumerator("navigator:browser");
  while (e.hasMoreElements()) {
    if (!e.getNext().closed)
      ++count;
  }
  return count;
}

function test() {
  
  is(browserWindowsCount(), 1, "Only one browser window should be open initially");

  
  let ss = Cc["@mozilla.org/browser/sessionstore;1"].getService(Ci.nsISessionStore);
  let ioService = Cc["@mozilla.org/network/io-service;1"].getService(Ci.nsIIOService);

  waitForExplicitFinish();

  const REFERRER1 = "http://www.example.net/?" + Date.now();
  const REFERRER2 = "http://www.example.net/?" + Math.random();

  let tab = gBrowser.addTab();
  gBrowser.selectedTab = tab;

  let browser = tab.linkedBrowser;
  browser.addEventListener("load", function() {
    browser.removeEventListener("load", arguments.callee, true);

    let tabState = JSON.parse(ss.getTabState(tab));
    is(tabState.entries[0].referrer,  REFERRER1,
       "Referrer retrieved via getTabState matches referrer set via loadURI.");

    tabState.entries[0].referrer = REFERRER2;
    ss.setTabState(tab, JSON.stringify(tabState));

    tab.addEventListener("SSTabRestored", function() {
      tab.removeEventListener("SSTabRestored", arguments.callee, true);
      is(window.content.document.referrer, REFERRER2, "document.referrer matches referrer set via setTabState.");

      gBrowser.removeTab(tab);
      let newTab = ss.undoCloseTab(window, 0);
      newTab.addEventListener("SSTabRestored", function() {
        newTab.removeEventListener("SSTabRestored", arguments.callee, true);

        is(window.content.document.referrer, REFERRER2, "document.referrer is still correct after closing and reopening the tab.");
        gBrowser.removeTab(newTab);

        is(browserWindowsCount(), 1, "Only one browser window should be open eventually");
        finish();
      }, true);
    }, true);
  },true);

  let referrerURI = ioService.newURI(REFERRER1, null, null);
  browser.loadURI("http://www.example.net", referrerURI, null);
}
