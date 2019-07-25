







































let tab;
function test() {
  waitForExplicitFinish();

  gPrefService.setIntPref("browser.sessionstore.max_tabs_undo", 0);
  gPrefService.clearUserPref("browser.sessionstore.max_tabs_undo");

  is(ss.getClosedTabCount(window), 0, "should be no closed tabs");

  gBrowser.tabContainer.addEventListener("TabOpen", onTabOpen, true);

  tab = gBrowser.addTab();
}

function onTabOpen(aEvent) {
  gBrowser.tabContainer.removeEventListener("TabOpen", onTabOpen, true);

  
  executeSoon(function() {
    is(gBrowser.browsers[1].currentURI.spec, "about:blank",
       "we will be removing an about:blank tab");

    gBrowser.removeTab(tab);

    is(ss.getClosedTabCount(window), 0, "should still be no closed tabs");

    executeSoon(finish);
  });
}
