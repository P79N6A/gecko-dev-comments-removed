







add_task(function* () {
  while (ss.getClosedTabCount(window)) {
    ss.forgetClosedTab(window, 0);
  }

  is(ss.getClosedTabCount(window), 0, "should be no closed tabs");

  let tab = gBrowser.addTab("about:privatebrowsing");
  let browser = tab.linkedBrowser;
  yield promiseBrowserLoaded(browser);

  is(gBrowser.browsers[1].currentURI.spec, "about:privatebrowsing",
     "we will be removing an about:privatebrowsing tab");

  gBrowser.removeTab(tab);
  is(ss.getClosedTabCount(window), 0, "should still be no closed tabs");
});
