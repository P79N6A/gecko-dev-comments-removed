








































function test() {
  const kTestPage1 = "data:text/html,<body%20onbeforeunload='return%20false;'>first</body>";
  const kTestPage2 = "data:text/html,<body%20onbeforeunload='return%20false;'>second</body>";
  let pb = Cc["@mozilla.org/privatebrowsing;1"]
             .getService(Ci.nsIPrivateBrowsingService);

  let promptHelper = {
    rejectDialog: 0,
    acceptDialog: 0,
    confirmCalls: 0,

    observe: function(aSubject, aTopic, aData) {
      let dialogWin = aSubject.QueryInterface(Ci.nsIDOMWindow);
      this.confirmCalls++;
      let button;
      if (this.acceptDialog-- > 0)
        button = dialogWin.document.documentElement.getButton("accept").click();
      else if (this.rejectDialog-- > 0)
        button = dialogWin.document.documentElement.getButton("cancel").click();
    }
  };

  Cc["@mozilla.org/observer-service;1"]
    .getService(Ci.nsIObserverService)
    .addObserver(promptHelper, "common-dialog-loaded", false);

  waitForExplicitFinish();
  let browser1 = gBrowser.getBrowserForTab(gBrowser.addTab());
  browser1.addEventListener("load", function() {
    browser1.removeEventListener("load", arguments.callee, true);

    let browser2 = gBrowser.getBrowserForTab(gBrowser.addTab());
    browser2.addEventListener("load", function() {
      browser2.removeEventListener("load", arguments.callee, true);

      promptHelper.rejectDialog = 1;
      pb.privateBrowsingEnabled = true;

      ok(!pb.privateBrowsingEnabled, "Private browsing mode should not have been activated");
      is(promptHelper.confirmCalls, 1, "Only one confirm box should be shown");
      is(gBrowser.tabContainer.childNodes.length, 3,
         "No tabs should be closed because private browsing mode transition was canceled");
      is(gBrowser.getBrowserForTab(gBrowser.tabContainer.firstChild).currentURI.spec, "about:blank",
         "The first tab should be a blank tab");
      is(gBrowser.getBrowserForTab(gBrowser.tabContainer.firstChild.nextSibling).currentURI.spec, kTestPage1,
         "The middle tab should be the same one we opened");
      is(gBrowser.getBrowserForTab(gBrowser.tabContainer.lastChild).currentURI.spec, kTestPage2,
         "The last tab should be the same one we opened");
      is(promptHelper.rejectDialog, 0, "Only one confirm dialog should have been rejected");

      promptHelper.confirmCalls = 0;
      promptHelper.acceptDialog = 2;
      pb.privateBrowsingEnabled = true;

      ok(pb.privateBrowsingEnabled, "Private browsing mode should have been activated");
      is(promptHelper.confirmCalls, 2, "Only two confirm boxes should be shown");
      is(gBrowser.tabContainer.childNodes.length, 1,
         "Incorrect number of tabs after transition into private browsing");
      gBrowser.selectedBrowser.addEventListener("load", function() {
        gBrowser.selectedBrowser.removeEventListener("load", arguments.callee, true);

        is(gBrowser.selectedBrowser.currentURI.spec, "about:privatebrowsing",
           "Incorrect page displayed after private browsing transition");
        is(promptHelper.acceptDialog, 0, "Two confirm dialogs should have been accepted");

        gBrowser.selectedBrowser.addEventListener("load", function() {
          gBrowser.selectedBrowser.removeEventListener("load", arguments.callee, true);

          gBrowser.selectedTab = gBrowser.addTab();
          gBrowser.selectedBrowser.addEventListener("load", function() {
            gBrowser.selectedBrowser.removeEventListener("load", arguments.callee, true);

            promptHelper.confirmCalls = 0;
            promptHelper.rejectDialog = 1;
            pb.privateBrowsingEnabled = false;

            ok(pb.privateBrowsingEnabled, "Private browsing mode should not have been deactivated");
            is(promptHelper.confirmCalls, 1, "Only one confirm box should be shown");
            is(gBrowser.tabContainer.childNodes.length, 2,
               "No tabs should be closed because private browsing mode transition was canceled");
            is(gBrowser.getBrowserForTab(gBrowser.tabContainer.firstChild).currentURI.spec, kTestPage1,
               "The first tab should be the same one we opened");
            is(gBrowser.getBrowserForTab(gBrowser.tabContainer.lastChild).currentURI.spec, kTestPage2,
               "The last tab should be the same one we opened");
            is(promptHelper.rejectDialog, 0, "Only one confirm dialog should have been rejected");

            promptHelper.confirmCalls = 0;
            promptHelper.acceptDialog = 2;
            pb.privateBrowsingEnabled = false;

            ok(!pb.privateBrowsingEnabled, "Private browsing mode should have been deactivated");
            is(promptHelper.confirmCalls, 2, "Only two confirm boxes should be shown");
            is(gBrowser.tabContainer.childNodes.length, 3,
               "Incorrect number of tabs after transition into private browsing");

            let loads = 0;
            function waitForLoad(event) {
              gBrowser.removeEventListener("load", arguments.callee, true);

              if (++loads != 3)
                return;

              is(gBrowser.getBrowserForTab(gBrowser.tabContainer.firstChild).currentURI.spec, "about:blank",
                 "The first tab should be a blank tab");
              is(gBrowser.getBrowserForTab(gBrowser.tabContainer.firstChild.nextSibling).currentURI.spec, kTestPage1,
                 "The middle tab should be the same one we opened");
              is(gBrowser.getBrowserForTab(gBrowser.tabContainer.lastChild).currentURI.spec, kTestPage2,
                 "The last tab should be the same one we opened");
                      is(promptHelper.acceptDialog, 0, "Two confirm dialogs should have been accepted");
              is(promptHelper.acceptDialog, 0, "Two prompts should have been raised");

              promptHelper.acceptDialog = 2;
              gBrowser.removeTab(gBrowser.tabContainer.lastChild);
              gBrowser.removeTab(gBrowser.tabContainer.lastChild);

              finish();
            }
            for (let i = 0; i < gBrowser.browsers.length; ++i)
              gBrowser.browsers[i].addEventListener("load", waitForLoad, true);
          }, true);
          gBrowser.selectedBrowser.loadURI(kTestPage2);
        }, true);
        gBrowser.selectedBrowser.loadURI(kTestPage1);
      }, true);
    }, true);
    browser2.loadURI(kTestPage2);
  }, true);
  browser1.loadURI(kTestPage1);
}
