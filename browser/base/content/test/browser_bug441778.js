








function test() {
  waitForExplicitFinish();

  const TEST_PAGE_URL = 'data:text/html,<body><iframe src=""></iframe></body>';
  const TEST_IFRAME_URL = "http://test2.example.org/";

  Task.spawn(function () {
    
    let tab = gBrowser.addTab();
    yield FullZoomHelper.selectTabAndWaitForLocationChange(tab);

    let testBrowser = tab.linkedBrowser;

    yield FullZoomHelper.load(tab, TEST_PAGE_URL);

    
    
    yield FullZoomHelper.enlarge();
    var zoomLevel = ZoomManager.zoom;

    
    let deferred = Promise.defer();
    executeSoon(function () {
      testBrowser.addEventListener("load", function (e) {
        testBrowser.removeEventListener("load", arguments.callee, true);

        is(e.target.defaultView.location, TEST_IFRAME_URL, "got the load event for the iframe");
        is(ZoomManager.zoom, zoomLevel, "zoom is retained after sub-document load");

        gBrowser.removeCurrentTab();
        deferred.resolve();
      }, true);
      content.document.querySelector("iframe").src = TEST_IFRAME_URL;
    });
    yield deferred.promise;
  }).then(finish, FullZoomHelper.failAndContinue(finish));
}
