




































function test() {
  waitForExplicitFinish();

  let tabOne = gBrowser.addTab("about:blank");
  let tabTwo = gBrowser.addTab("http://mochi.test:8888/");

  gBrowser.selectedTab = tabTwo;

  var browser = gBrowser.getBrowserForTab(tabTwo);
  browser.addEventListener("load", function() {
    gBrowser.showOnlyTheseTabs([tabTwo]);

    is(gBrowser.visibleTabs.length, 1, "Only one tab is visible");

    let uris = PlacesCommandHook._getUniqueTabInfo();
    is(uris.length, 1, "Only one uri is returned");

    is(uris[0].spec, tabTwo.linkedBrowser.currentURI.spec, "It's the correct URI");

    gBrowser.removeTab(tabOne);
    gBrowser.removeTab(tabTwo);
    Array.forEach(gBrowser.tabs, function(tab) {
      tab.hidden = false;
    });

    finish();
  }, true);
}
