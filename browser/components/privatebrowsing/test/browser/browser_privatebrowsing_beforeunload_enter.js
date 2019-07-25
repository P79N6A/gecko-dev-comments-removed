








































function test() {
  const TEST_PAGE_1 = "data:text/html,<body%20onbeforeunload='return%20false;'>first</body>";
  const TEST_PAGE_2 = "data:text/html,<body%20onbeforeunload='return%20false;'>second</body>";
  let pb = Cc["@mozilla.org/privatebrowsing;1"]
             .getService(Ci.nsIPrivateBrowsingService);

  let rejectDialog = 0;
  let acceptDialog = 0;
  let confirmCalls = 0;
  function promptObserver(aSubject, aTopic, aData) {
    let dialogWin = aSubject.QueryInterface(Ci.nsIDOMWindow);
    confirmCalls++;
    if (acceptDialog-- > 0)
      dialogWin.document.documentElement.getButton("accept").click();
    else if (rejectDialog-- > 0)
      dialogWin.document.documentElement.getButton("cancel").click();
  }

  Services.obs.addObserver(promptObserver, "common-dialog-loaded", false);

  waitForExplicitFinish();
  let browser1 = gBrowser.getBrowserForTab(gBrowser.addTab());
  browser1.addEventListener("load", function() {
    browser1.removeEventListener("load", arguments.callee, true);

    let browser2 = gBrowser.getBrowserForTab(gBrowser.addTab());
    browser2.addEventListener("load", function() {
      browser2.removeEventListener("load", arguments.callee, true);

      rejectDialog = 1;
      pb.privateBrowsingEnabled = true;

      ok(!pb.privateBrowsingEnabled, "Private browsing mode should not have been activated");
      is(confirmCalls, 1, "Only one confirm box should be shown");
      is(gBrowser.tabs.length, 3,
         "No tabs should be closed because private browsing mode transition was canceled");
      is(gBrowser.getBrowserForTab(gBrowser.tabContainer.firstChild).currentURI.spec, "about:blank",
         "The first tab should be a blank tab");
      is(gBrowser.getBrowserForTab(gBrowser.tabContainer.firstChild.nextSibling).currentURI.spec, TEST_PAGE_1,
         "The middle tab should be the same one we opened");
      is(gBrowser.getBrowserForTab(gBrowser.tabContainer.lastChild).currentURI.spec, TEST_PAGE_2,
         "The last tab should be the same one we opened");
      is(rejectDialog, 0, "Only one confirm dialog should have been rejected");

      confirmCalls = 0;
      acceptDialog = 2;
      pb.privateBrowsingEnabled = true;

      ok(pb.privateBrowsingEnabled, "Private browsing mode should have been activated");
      is(confirmCalls, 2, "Only two confirm boxes should be shown");
      is(gBrowser.tabs.length, 1,
         "Incorrect number of tabs after transition into private browsing");
      gBrowser.selectedBrowser.addEventListener("load", function() {
        gBrowser.selectedBrowser.removeEventListener("load", arguments.callee, true);

        is(gBrowser.currentURI.spec, "about:privatebrowsing",
           "Incorrect page displayed after private browsing transition");
        is(acceptDialog, 0, "Two confirm dialogs should have been accepted");

        gBrowser.addTab();
        gBrowser.removeTab(gBrowser.selectedTab);
        Services.prefs.setBoolPref("browser.privatebrowsing.keep_current_session", true);
        pb.privateBrowsingEnabled = false;
        Services.prefs.clearUserPref("browser.privatebrowsing.keep_current_session");
        Services.obs.removeObserver(promptObserver, "common-dialog-loaded", false);
        gBrowser.getBrowserAtIndex(gBrowser.tabContainer.selectedIndex).contentWindow.focus();
        finish();
      }, true);
    }, true);
    browser2.loadURI(TEST_PAGE_2);
  }, true);
  browser1.loadURI(TEST_PAGE_1);
}
