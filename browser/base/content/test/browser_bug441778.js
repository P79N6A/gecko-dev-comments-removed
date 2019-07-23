









































function test() {
  waitForExplicitFinish();

  const TEST_PAGE_URL = 'data:text/html,<body><iframe src=""></iframe></body>';
  const TEST_IFRAME_URL = "http://test2.example.org/";

  
  gBrowser.selectedTab = gBrowser.addTab();
  let testBrowser = gBrowser.selectedBrowser;

  testBrowser.addEventListener("load", function () {
    testBrowser.removeEventListener("load", arguments.callee, true);

    
    
    FullZoom.enlarge();
    var zoomLevel = ZoomManager.zoom;

    
    executeSoon(function () {
      testBrowser.addEventListener("load", function (e) {
        testBrowser.removeEventListener("load", arguments.callee, true);

        is(e.target.defaultView.location, TEST_IFRAME_URL, "got the load event for the iframe");
        is(ZoomManager.zoom, zoomLevel, "zoom is retained after sub-document load");

        gBrowser.removeCurrentTab();
        finish();
      }, true);
      content.document.querySelector("iframe").src = TEST_IFRAME_URL;
    });
  }, true);

  content.location = TEST_PAGE_URL;
}
