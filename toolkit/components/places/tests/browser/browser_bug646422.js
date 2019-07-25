









































function test() {
  waitForExplicitFinish();

  let tab = gBrowser.addTab('http://example.com');
  let tabBrowser = tab.linkedBrowser;

  tabBrowser.addEventListener('load', function(aEvent) {
    tabBrowser.removeEventListener('load', arguments.callee, true);

    
    let cw = tabBrowser.contentWindow;
    ok(cw.document.title, 'Content window should initially have a title.');
    cw.history.pushState('', '', 'new_page');
  }, true);

  let observer = {
    onTitleChanged: function(uri, title) {
      
      
      if (/new_page$/.test(uri.spec)) {
        is(title, tabBrowser.contentWindow.document.title,
           'Title after pushstate.');
        PlacesUtils.history.removeObserver(this);
        gBrowser.removeTab(tab);
        finish();
      }
    },

    onBeginUpdateBatch: function() { },
    onEndUpdateBatch: function() { },
    onVisit: function() { },
    onBeforeDeleteURI: function() { },
    onDeleteURI: function() { },
    onClearHistory: function() { },
    onPageChanged: function() { },
    onDeleteVisits: function() { },
    QueryInterface: XPCOMUtils.generateQI([Ci.nsINavHistoryObserver])
  };

  PlacesUtils.history.addObserver(observer, false);
}
