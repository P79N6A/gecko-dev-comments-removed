



function test() {
    waitForExplicitFinish();

    var pagetitle = "Page Title for Bug 503832";
    var pageurl = "http://localhost:8888/browser/docshell/test/browser/file_bug503832.html";
    var fragmenturl = "http://localhost:8888/browser/docshell/test/browser/file_bug503832.html#firefox";

    
    var historyObserver = {
        onBeginUpdateBatch: function() {},
        onEndUpdateBatch: function() {},
        onVisit: function(aURI, aVisitID, aTime, aSessionId, aReferringId,
                          aTransitionType, _added) {},
        onTitleChanged: function(aURI, aPageTitle) {
            aURI = aURI.spec;
            switch (aURI) {
            case pageurl:
                is(aPageTitle, pagetitle, "Correct page title for " + aURI);
                return;
            case fragmenturl:
                is(aPageTitle, pagetitle, "Correct page title for " + aURI);
                
                
                
                finish();
            }
        },
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

    var historyService = Cc["@mozilla.org/browser/nav-history-service;1"]
                         .getService(Ci.nsINavHistoryService);
    historyService.addObserver(historyObserver, false);

    

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


    function onPageLoad() {
        gBrowser.selectedBrowser.removeEventListener(
            "DOMContentLoaded", onPageLoad, true);

        
        EventUtils.sendMouseEvent({type:'click'}, 'firefox-link',
                                  gBrowser.selectedBrowser.contentWindow);

        
        setTimeout(function() {
                       gBrowser.removeCurrentTab();
                   }, 100);

        
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
