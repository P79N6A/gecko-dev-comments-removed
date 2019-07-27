



function test() {
  

  waitForExplicitFinish();

  const REFERRER1 = "http://example.org/?" + Date.now();
  const REFERRER2 = "http://example.org/?" + Math.random();

  let tab = gBrowser.addTab();
  gBrowser.selectedTab = tab;

  let browser = tab.linkedBrowser;
  promiseBrowserLoaded(browser).then(() => {
    let tabState = JSON.parse(ss.getTabState(tab));
    is(tabState.entries[0].referrer,  REFERRER1,
       "Referrer retrieved via getTabState matches referrer set via loadURI.");

    tabState.entries[0].referrer = REFERRER2;

    promiseTabState(tab, tabState).then(() => {
      is(window.content.document.referrer, REFERRER2, "document.referrer matches referrer set via setTabState.");

      gBrowser.removeTab(tab);

      let newTab = ss.undoCloseTab(window, 0);
      promiseTabRestored(newTab).then(() => {
        is(window.content.document.referrer, REFERRER2, "document.referrer is still correct after closing and reopening the tab.");
        gBrowser.removeTab(newTab);

        finish();
      });
    });
  });

  let referrerURI = Services.io.newURI(REFERRER1, null, null);
  browser.loadURI("http://example.org", referrerURI, null);
}
