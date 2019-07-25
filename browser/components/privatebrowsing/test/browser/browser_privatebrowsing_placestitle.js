







































function test() {
  
  let pb = Cc["@mozilla.org/privatebrowsing;1"].
           getService(Ci.nsIPrivateBrowsingService);
  let cm = Cc["@mozilla.org/cookiemanager;1"].
           getService(Ci.nsICookieManager);
  waitForExplicitFinish();
  ignoreAllUncaughtExceptions();

  const TEST_URL = "http://mochi.test:8888/browser/browser/components/privatebrowsing/test/browser/title.sjs";

  function waitForCleanup(aCallback) {
    
    cm.removeAll();
    
    waitForClearHistory(aCallback);
  }

  let observer = {
    pass: 1,
    onTitleChanged: function(aURI, aPageTitle) {
      if (aURI.spec != TEST_URL)
        return;
      switch (this.pass++) {
      case 1: 
        is(aPageTitle, "No Cookie", "The page should be loaded without any cookie for the first time");
        gBrowser.selectedTab = gBrowser.addTab(TEST_URL);
        break;
      case 2: 
        is(aPageTitle, "Cookie", "The page should be loaded with a cookie for the second time");
        waitForCleanup(function () {
          gBrowser.selectedTab = gBrowser.addTab(TEST_URL);
        });
        break;
      case 3: 
        is(aPageTitle, "No Cookie", "The page should be loaded without any cookie again");
        
        pb.privateBrowsingEnabled = true;
        gBrowser.selectedTab = gBrowser.addTab(TEST_URL);
        executeSoon(function() {
          PlacesUtils.history.removeObserver(observer);
          pb.privateBrowsingEnabled = false;
          while (gBrowser.browsers.length > 1) {
            gBrowser.removeCurrentTab();
          }
          waitForCleanup(finish);
        });
        break;
      default:
        ok(false, "Unexpected pass: " + (this.pass - 1));
      }
    },

    onBeginUpdateBatch: function () {},
    onEndUpdateBatch: function () {},
    onVisit: function () {},
    onBeforeDeleteURI: function () {},
    onDeleteURI: function () {},
    onClearHistory: function () {},
    onPageChanged: function () {},
    onDeleteVisits: function() {},

    QueryInterface: XPCOMUtils.generateQI([Ci.nsINavHistoryObserver])
  };
  PlacesUtils.history.addObserver(observer, false);

  waitForCleanup(function () {
    gBrowser.selectedTab = gBrowser.addTab(TEST_URL);
  });
}
