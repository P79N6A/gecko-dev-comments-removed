




































function test()
{
  const TEST_URI = "http://example.com/tests/toolkit/components/places/tests/browser/399606-window.location.href.html";

  var histsvc = Cc["@mozilla.org/browser/nav-history-service;1"].
                getService(Ci.nsINavHistoryService);

  
  var observer = {
    onBeginUpdateBatch: function() {},
    onEndUpdateBatch: function() {},
    onVisit: function(aURI, aVisitID, aTime, aSessionID, aReferringID,
                      aTransitionType) {
      info("onVisit: " + aURI.spec);
      confirm_results();

      histsvc.removeObserver(observer, false);
      win.content.document.location.href = "about:blank";
      finish();
    },
    onTitleChanged: function(aURI, aPageTitle) {},
    onBeforeDeleteURI: function(aURI) {},
    onDeleteURI: function(aURI) {},
    onClearHistory: function() {},
    onPageChanged: function(aURI, aWhat, aValue) {},
    onPageExpired: function(aURI, aVisitTime, aWholeEntry) {},
    QueryInterface: function(iid) {
      if (iid.equals(Ci.nsINavHistoryObserver) ||
          iid.equals(Ci.nsISupports)) {
        return this;
      }
      throw Cr.NS_ERROR_NO_INTERFACE;
    }
  };
  
  histsvc.addObserver(observer, false);
  
  
  var loadCount = 0;
  function confirm_results() {
    var options = histsvc.getNewQueryOptions();
    options.resultType = options.RESULTS_AS_VISIT;
    options.includeHidden = true;
    var query = histsvc.getNewQuery();
    var uri = Cc["@mozilla.org/network/io-service;1"].
              getService(Ci.nsIIOService).
              newURI(TEST_URI, null, null);
    info("query uri is " + uri.spec);
    query.uri = uri;
    var result = histsvc.executeQuery(query, options);
    var root = result.root;
    root.containerOpen = true;
    var cc = root.childCount;
    is(cc, 1, "Visit count is what we expect");
    ok(loadCount > 1, "Load count is greater than 1");
    root.containerOpen = false;
  }

  var wm = Cc["@mozilla.org/appshell/window-mediator;1"].
           getService(Ci.nsIWindowMediator);
  var win = wm.getMostRecentWindow("navigator:browser");

  function handleLoad(aEvent) {
    info("location is " + aEvent.originalTarget.location.href);
    loadCount++;
    info("new load count is " + loadCount);

    if (loadCount == 3)
      win.getBrowser().removeEventListener("DOMContentLoaded", handleLoad, true);
  }

  win.getBrowser().addEventListener("DOMContentLoaded", handleLoad, true);

  
  win.content.document.location.href = TEST_URI;

  
  waitForExplicitFinish();
}
