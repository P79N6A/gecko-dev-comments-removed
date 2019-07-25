









function test() {
  const testDir = "http://mochi.test:8888/browser/docshell/test/browser/";
  const origURL = testDir + "file_bug655270.html";
  const newURL  = origURL + '?new_page';

  const faviconURL = testDir + "favicon_bug655270.ico";

  waitForExplicitFinish();

  let tab = gBrowser.addTab(origURL);

  
  
  
  

  let observer = {
    onPageChanged: function(aURI, aWhat, aValue) {
      if (aWhat != Ci.nsINavHistoryObserver.ATTRIBUTE_FAVICON)
        return;

      if (aURI.spec == origURL) {
        is(aValue, faviconURL, 'FaviconURL for original URI');
        tab.linkedBrowser.contentWindow.history.pushState('', '', '?new_page');
      }

      if (aURI.spec == newURL) {
        is(aValue, faviconURL, 'FaviconURL for new URI');
        gBrowser.removeTab(tab);
        PlacesUtils.history.removeObserver(this);
        finish();
      }
    },

    onBeginUpdateBatch: function() { },
    onEndUpdateBatch: function() { },
    onVisit: function() { },
    onTitleChanged: function() { },
    onBeforeDeleteURI: function() { },
    onDeleteURI: function() { },
    onClearHistory: function() { },
    onDeleteVisits: function() { },
    QueryInterface: XPCOMUtils.generateQI([Ci.nsINavHistoryObserver])
  };

  PlacesUtils.history.addObserver(observer, false);
}
