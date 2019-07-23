





































function test() {
  
  waitForExplicitFinish();
  const LAZY_ADD_TIMER = 3000;

  var URIs = [
    "http://example.com/tests/toolkit/components/places/tests/browser/399606-window.location.href.html",
    "http://example.com/tests/toolkit/components/places/tests/browser/399606-history.go-0.html",
    "http://example.com/tests/toolkit/components/places/tests/browser/399606-location.replace.html",
    "http://example.com/tests/toolkit/components/places/tests/browser/399606-location.reload.html",
    "http://example.com/tests/toolkit/components/places/tests/browser/399606-httprefresh.html",
    "http://example.com/tests/toolkit/components/places/tests/browser/399606-window.location.html",
  ];
  var hs = Cc["@mozilla.org/browser/nav-history-service;1"].
           getService(Ci.nsINavHistoryService);

  
  var historyObserver = {
    visitCount: Array(),
    onBeginUpdateBatch: function() {},
    onEndUpdateBatch: function() {},
    onVisit: function(aURI, aVisitID, aTime, aSessionID, aReferringID,
                      aTransitionType) {
      info("Received onVisit: " + aURI.spec);
      if (aURI.spec in this.visitCount)
        this.visitCount[aURI.spec]++;
      else
        this.visitCount[aURI.spec] = 1;
    },
    onTitleChanged: function(aURI, aPageTitle) {},
    onBeforeDeleteURI: function(aURI) {},
    onDeleteURI: function(aURI) {},
    onClearHistory: function() {},
    onPageChanged: function(aURI, aWhat, aValue) {},
    onPageExpired: function(aURI, aVisitTime, aWholeEntry) {},
    QueryInterface: XPCOMUtils.generateQI([Ci.nsINavHistoryObserver])
  };
  hs.addObserver(historyObserver, false);

  function confirm_results() {
    hs.removeObserver(historyObserver, false);
    for (let aURI in historyObserver.visitCount) {
      is(historyObserver.visitCount[aURI], 1, "onVisit has been received right number of times for " + aURI);
    }
    hs.QueryInterface(Ci.nsIBrowserHistory).removeAllPages();
    finish();
  }

  var loadCount = 0;
  function handleLoad(aEvent) {
    loadCount++;
    info("new load count is " + loadCount);

    if (loadCount == 3) {
      window.getBrowser().removeEventListener("DOMContentLoaded", handleLoad, true);
      window.content.document.location.href = "about:blank";
      executeSoon(check_next_uri);
    }
  }

  function check_next_uri() {
    if (URIs.length) {
      let uri = URIs.shift();
      loadCount = 0;
      window.getBrowser().addEventListener("DOMContentLoaded", handleLoad, true);
      window.content.document.location.href = uri;
    }
    else {
      setTimeout(confirm_results, LAZY_ADD_TIMER * 2);
    }
  }
  executeSoon(check_next_uri);
}
