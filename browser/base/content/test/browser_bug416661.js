var tabElm, zoomLevel;
function start_test_prefNotSet() {
  Task.spawn(function () {
    is(ZoomManager.zoom, 1, "initial zoom level should be 1");
    yield FullZoomHelper.enlarge();

    
    zoomLevel = ZoomManager.zoom;
    isnot(zoomLevel, 1, "zoom level should have changed");

    yield FullZoomHelper.load(gBrowser.selectedTab, "http://mochi.test:8888/browser/browser/base/content/test/moz.png");
  }).then(continue_test_prefNotSet, FullZoomHelper.failAndContinue(finish));
}

function continue_test_prefNotSet () {
  Task.spawn(function () {
    is(ZoomManager.zoom, 1, "zoom level pref should not apply to an image");
    yield FullZoomHelper.reset();

    yield FullZoomHelper.load(gBrowser.selectedTab, "http://mochi.test:8888/browser/browser/base/content/test/zoom_test.html");
  }).then(end_test_prefNotSet, FullZoomHelper.failAndContinue(finish));
}

function end_test_prefNotSet() {
  Task.spawn(function () {
    is(ZoomManager.zoom, zoomLevel, "the zoom level should have persisted");

    
    yield FullZoomHelper.reset();
    gBrowser.removeCurrentTab();
  }).then(finish, FullZoomHelper.failAndContinue(finish));
}

function test() {
  waitForExplicitFinish();

  Task.spawn(function () {
    tabElm = gBrowser.addTab();
    yield FullZoomHelper.selectTabAndWaitForLocationChange(tabElm);
    yield FullZoomHelper.load(tabElm, "http://mochi.test:8888/browser/browser/base/content/test/zoom_test.html");
  }).then(start_test_prefNotSet, FullZoomHelper.failAndContinue(finish));
}
