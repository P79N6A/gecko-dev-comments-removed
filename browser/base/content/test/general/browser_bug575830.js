


"use strict";

function test() {
  let tab1, tab2;
  const TEST_IMAGE = "http://example.org/browser/browser/base/content/test/general/moz.png";

  waitForExplicitFinish();
  registerCleanupFunction(function cleanup() {
    gBrowser.removeTab(tab1);
    gBrowser.removeTab(tab2);
  });

  Task.spawn(function () {
    tab1 = gBrowser.addTab();
    tab2 = gBrowser.addTab();
    yield FullZoomHelper.selectTabAndWaitForLocationChange(tab1);
    yield FullZoomHelper.load(tab1, TEST_IMAGE);

    is(ZoomManager.zoom, 1, "initial zoom level for first should be 1");

    FullZoom.enlarge();
    let zoom = ZoomManager.zoom;
    isnot(zoom, 1, "zoom level should have changed");

    yield FullZoomHelper.selectTabAndWaitForLocationChange(tab2);
    is(ZoomManager.zoom, 1, "initial zoom level for second tab should be 1");

    yield FullZoomHelper.selectTabAndWaitForLocationChange(tab1);
    is(ZoomManager.zoom, zoom, "zoom level for first tab should not have changed");
  }).then(finish, FullZoomHelper.failAndContinue(finish));
}
