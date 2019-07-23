









































let testPage = 'data:text/html,<body><iframe id="a" src=""></iframe></body>';

function test() {
  waitForExplicitFinish();

  
  
  
  let zoomLevel;

  
  gBrowser.selectedTab = gBrowser.addTab();
  let testBrowser = gBrowser.selectedBrowser;

  let finishTest = function() {
    testBrowser.removeProgressListener(progressListener);
    is(ZoomManager.zoom, zoomLevel, "zoom is retained after sub-document load");
    gBrowser.removeCurrentTab();
    finish();
  };

  let progressListener = {
    QueryInterface: XPCOMUtils.generateQI([Ci.nsIWebProgressListener,
                                           Ci.nsISupportsWeakReference]),
    onStateChange: function() {},
    onProgressChange: function() {},
    onLocationChange: function() {
      window.setTimeout(finishTest, 0);
    },
    onStatusChange: function() {},
    onSecurityChange: function() {}
  };

  let continueTest = function() {
    
    
    FullZoom.enlarge();
    zoomLevel = ZoomManager.zoom;

    
    
    testBrowser.addProgressListener(progressListener);

    
    content.document.getElementById("a").src = "http://test2.example.org/";
  };

  
  
  
  
  let continueListener = function() {
    window.setTimeout(continueTest, 0);
    
    
    testBrowser.removeEventListener("load", continueListener, true);
  };
  testBrowser.addEventListener("load", continueListener, true);

  
  testBrowser.contentWindow.location = testPage;
}
