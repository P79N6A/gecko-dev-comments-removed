







































function test() {
  
  let pb = Cc["@mozilla.org/privatebrowsing;1"].
           getService(Ci.nsIPrivateBrowsingService);
  let bhist = Cc["@mozilla.org/browser/global-history;2"].
              getService(Ci.nsIBrowserHistory);
  let histsvc = Cc["@mozilla.org/browser/nav-history-service;1"].
                getService(Ci.nsINavHistoryService).
                QueryInterface(Ci.nsPIPlacesDatabase);
  let cm = Cc["@mozilla.org/cookiemanager;1"].
           getService(Ci.nsICookieManager);
  waitForExplicitFinish();

  const TEST_URL = "http://localhost:8888/browser/browser/components/privatebrowsing/test/browser/title.sjs";

  function cleanup() {
    
    bhist.removeAllPages();
    
    cm.removeAll();
  }
  cleanup();

  let observer = {
    pass: 1,
    onBeginUpdateBatch: function() {
    },
    onEndUpdateBatch: function() {
    },
    onVisit: function(aURI, aVisitID, aTime, aSessionId, aReferringId,
                      aTransitionType, _added) {
    },
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
        cleanup();
        gBrowser.selectedTab = gBrowser.addTab(TEST_URL);
        break;
      case 3: 
        is(aPageTitle, "No Cookie", "The page should be loaded without any cookie again");
        
        pb.privateBrowsingEnabled = true;
        gBrowser.selectedTab = gBrowser.addTab(TEST_URL);
        executeSoon(function() {
          histsvc.removeObserver(observer);
          pb.privateBrowsingEnabled = false;
          while (gBrowser.browsers.length > 1)
            gBrowser.removeCurrentTab();
          cleanup();
          finish();
        });
        break;
      default:
        ok(false, "Unexpected pass: " + (this.pass - 1));
      }
    },
    onBeforeDeleteURI: function(aURI) {
    },
    onDeleteURI: function(aURI) {
    },
    onClearHistory: function() {
    },
    onPageChanged: function(aURI, aWhat, aValue) {
    },
    onPageExpired: function(aURI, aVisitTime, aWholeEntry) {
    },
    QueryInterface: function(iid) {
      if (iid.equals(Ci.nsINavHistoryObserver) ||
          iid.equals(Ci.nsISupports)) {
        return this;
      }
      throw Cr.NS_ERROR_NO_INTERFACE;
    }
  };
  histsvc.addObserver(observer, false);

  gBrowser.selectedTab = gBrowser.addTab(TEST_URL);
}
