








































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
  pb.privateBrowsingEnabled = true;

  waitForExplicitFinish();
  let browser1 = gBrowser.getBrowserForTab(gBrowser.addTab());
  browser1.addEventListener("load", function() {
    browser1.removeEventListener("load", arguments.callee, true);

    let browser2 = gBrowser.getBrowserForTab(gBrowser.addTab());
    browser2.addEventListener("load", function() {
      browser2.removeEventListener("load", arguments.callee, true);

      confirmCalls = 0;
      rejectDialog = 1;
      pb.privateBrowsingEnabled = false;

      ok(pb.privateBrowsingEnabled, "Private browsing mode should not have been deactivated");
      is(confirmCalls, 1, "Only one confirm box should be shown");
      is(gBrowser.tabs.length, 3,
         "No tabs should be closed because private browsing mode transition was canceled");
      is(gBrowser.getBrowserForTab(gBrowser.tabContainer.firstChild).currentURI.spec, "about:privatebrowsing",
         "The first tab should be the same one we opened");
      is(gBrowser.getBrowserForTab(gBrowser.tabContainer.lastChild).currentURI.spec, TEST_PAGE_2,
         "The last tab should be the same one we opened");
      is(rejectDialog, 0, "Only one confirm dialog should have been rejected");

      confirmCalls = 0;
      acceptDialog = 2;
      pb.privateBrowsingEnabled = false;

      ok(!pb.privateBrowsingEnabled, "Private browsing mode should have been deactivated");
      is(confirmCalls, 2, "Only two confirm boxes should be shown");
      is(gBrowser.tabs.length, 3,
         "Incorrect number of tabs after transition into private browsing");

      let loads = 0;
      function waitForLoad(event) {
        gBrowser.removeEventListener("load", arguments.callee, true);

        if (++loads != 3)
          return;

        is(gBrowser.getBrowserForTab(gBrowser.tabContainer.firstChild).currentURI.spec, "about:blank",
           "The first tab should be a blank tab");
        is(gBrowser.getBrowserForTab(gBrowser.tabContainer.firstChild.nextSibling).currentURI.spec, TEST_PAGE_1,
           "The middle tab should be the same one we opened");
        is(gBrowser.getBrowserForTab(gBrowser.tabContainer.lastChild).currentURI.spec, TEST_PAGE_2,
           "The last tab should be the same one we opened");
        is(acceptDialog, 0, "Two confirm dialogs should have been accepted");
        is(acceptDialog, 0, "Two prompts should have been raised");

        acceptDialog = 2;
        gBrowser.removeTab(gBrowser.tabContainer.lastChild);
        gBrowser.removeTab(gBrowser.tabContainer.lastChild);
        gBrowser.getBrowserAtIndex(gBrowser.tabContainer.selectedIndex).contentWindow.focus();

        Services.obs.removeObserver(promptObserver, "common-dialog-loaded", false);
        finish();
      }
      for (let i = 0; i < gBrowser.browsers.length; ++i)
        gBrowser.browsers[i].addEventListener("load", waitForLoad, true);
    }, true);
    browser2.loadURI(TEST_PAGE_2);
  }, true);
  browser1.loadURI(TEST_PAGE_1);
}
