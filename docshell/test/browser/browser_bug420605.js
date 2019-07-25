



function test() {
    waitForExplicitFinish();

    var pageurl = "http://mochi.test:8888/browser/docshell/test/browser/file_bug420605.html";
    var fragmenturl = "http://mochi.test:8888/browser/docshell/test/browser/file_bug420605.html#firefox";

    var historyService = Cc["@mozilla.org/browser/nav-history-service;1"]
                         .getService(Ci.nsINavHistoryService);

    

    function getNavHistoryEntry(aURI) {
        var options = historyService.getNewQueryOptions();
        options.queryType = Ci.nsINavHistoryQueryOptions.QUERY_TYPE_HISTORY;
        options.maxResults = 1;

        var query = historyService.getNewQuery();
        query.uri = aURI;

        var result = historyService.executeQuery(query, options);
        result.root.containerOpen = true;

        if (!result.root.childCount) {
            return null;
        }
        return result.root.getChild(0);
    }

    
    
    var originalFavicon;

    
    
    
    
    
    
    

    var _clickLinkTimes = 0;
    function clickLinkIfReady() {
      _clickLinkTimes++;
      if (_clickLinkTimes == 2) {
        EventUtils.sendMouseEvent({type:'click'}, 'firefox-link',
                                  gBrowser.selectedBrowser.contentWindow);
      }
    }

    
    var historyObserver = {
        onBeginUpdateBatch: function() {},
        onEndUpdateBatch: function() {},
        onVisit: function(aURI, aVisitID, aTime, aSessionId, aReferringId,
                          aTransitionType, _added) {},
        onTitleChanged: function(aURI, aPageTitle) {},
        onBeforeDeleteURI: function(aURI) {},
        onDeleteURI: function(aURI) {},
        onClearHistory: function() {},
        onPageChanged: function(aURI, aWhat, aValue) {
            if (aWhat != Ci.nsINavHistoryObserver.ATTRIBUTE_FAVICON) {
                return;
            }
            aURI = aURI.spec;
            switch (aURI) {
            case pageurl:
                ok(aValue, "Favicon value is not null for page without fragment.");
                originalFavicon = aValue;

                
                
                clickLinkIfReady();

                return;
            case fragmenturl:
                
                

                is(aValue, originalFavicon, "New favicon should be same as original favicon.");

                
                
                let info = getNavHistoryEntry(makeURI(aURI));
                ok(info, "There must be a history entry for the fragment.");
                ok(info.icon, "The history entry must have an associated favicon.");
                historyService.removeObserver(historyObserver, false);
                gBrowser.removeCurrentTab();
                finish();
            }
        },
        onPageExpired: function(aURI, aVisitTime, aWholeEntry) {},
        QueryInterface: function(iid) {
            if (iid.equals(Ci.nsINavHistoryObserver) ||
                iid.equals(Ci.nsISupports)) {
                return this;
            }
            throw Cr.NS_ERROR_NO_INTERFACE;
        }
    };
    historyService.addObserver(historyObserver, false);

    function onPageLoad() {
      gBrowser.selectedBrowser
              .removeEventListener("DOMContentLoaded", arguments.callee, true);
      clickLinkIfReady();
    }

    
    var info = getNavHistoryEntry(makeURI(pageurl));
    ok(!info, "The test page must not have been visited already.");
    info = getNavHistoryEntry(makeURI(fragmenturl));
    ok(!info, "The fragment test page must not have been visited already.");

    
    gBrowser.selectedTab = gBrowser.addTab();
    gBrowser.selectedBrowser.addEventListener(
        "DOMContentLoaded", onPageLoad, true);
    content.location = pageurl;
}
