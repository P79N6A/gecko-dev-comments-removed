


"use strict";

function test() {
  let tab1, tab2;
  const TEST_IMAGE = "http://example.org/browser/browser/base/content/test/moz.png";

  waitForExplicitFinish();
  registerCleanupFunction(function cleanup() {
    gBrowser.removeTab(tab1);
    gBrowser.removeTab(tab2);
  });

  tab1 = gBrowser.addTab(TEST_IMAGE);
  tab2 = gBrowser.addTab();
  gBrowser.selectedTab = tab1;

  tab1.linkedBrowser.addEventListener("load", function onload() {
    tab1.linkedBrowser.removeEventListener("load", onload, true);
    is(ZoomManager.zoom, 1, "initial zoom level for first should be 1");

    FullZoom.enlarge();
    let zoom = ZoomManager.zoom;
    isnot(zoom, 1, "zoom level should have changed");

    gBrowser.selectedTab = tab2;
    is(ZoomManager.zoom, 1, "initial zoom level for second tab should be 1");

    gBrowser.selectedTab = tab1;
    is(ZoomManager.zoom, zoom, "zoom level for first tab should not have changed");

    finish();
  }, true);
}

