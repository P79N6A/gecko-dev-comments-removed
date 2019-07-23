







































function test() {
  
  let pb = Cc["@mozilla.org/privatebrowsing;1"].
           getService(Ci.nsIPrivateBrowsingService);
  let obs = Cc["@mozilla.org/observer-service;1"].
            getService(Ci.nsIObserverService);
  let ww = Cc["@mozilla.org/embedcomp/window-watcher;1"].
           getService(Ci.nsIWindowWatcher);

  function runTest(aPBMode, aCallBack) {
    const kTestURL1 = "data:text/html,Test Page 1";
    let tab1 = gBrowser.addTab();
    gBrowser.selectedTab = tab1;
    let browser1 = gBrowser.getBrowserForTab(tab1);
    browser1.addEventListener("load", function() {
      browser1.removeEventListener("load", arguments.callee, true);

      let pageInfo1 = BrowserPageInfo();
      obs.addObserver({
        observe: function (aSubject, aTopic, aData) {
          obs.removeObserver(this, "page-info-dialog-loaded");

          const kTestURL2 = "data:text/plain,Test Page 2";
          let tab2 = gBrowser.addTab();
          gBrowser.selectedTab = tab2;
          let browser2 = gBrowser.getBrowserForTab(tab2);
          browser2.addEventListener("load", function () {
            browser2.removeEventListener("load", arguments.callee, true);

            let pageInfo2 = BrowserPageInfo();
            obs.addObserver({
              observe: function (aSubject, aTopic, aData) {
                obs.removeObserver(this, "page-info-dialog-loaded");

                ww.registerNotification({
                  observe: function (aSubject, aTopic, aData) {
                    is(aTopic, "domwindowclosed", "We should only receive window closed notifications");
                    let win = aSubject.QueryInterface(Ci.nsIDOMWindow);
                    if (win == pageInfo1) {
                      ok(true, "Page info 1 being closed as expected");
                      pageInfo1 = null;
                    }
                    else if (win == pageInfo2) {
                      ok(true, "Page info 2 being closed as expected");
                      pageInfo2 = null;
                    }
                    else
                      ok(false, "The closed window should be one of the two page info windows");

                    if (!pageInfo1 && !pageInfo2) {
                      ww.unregisterNotification(this);

                      aCallBack();
                    }
                  }
                });

                pb.privateBrowsingEnabled = aPBMode;
              }
            }, "page-info-dialog-loaded", false);
          }, true);
          browser2.loadURI(kTestURL2);
        }
      }, "page-info-dialog-loaded", false);
    }, true);
    browser1.loadURI(kTestURL1);
  }

  runTest(true, function() {
    runTest(false, function() {
      gBrowser.removeCurrentTab();
      gBrowser.removeCurrentTab();

      finish();
    });
  });

  waitForExplicitFinish();
}
