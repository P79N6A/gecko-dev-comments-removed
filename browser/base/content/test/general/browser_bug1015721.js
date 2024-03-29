


"use strict";

const TEST_PAGE = "http://example.org/browser/browser/base/content/test/general/zoom_test.html";

var gTab1, gTab2, gLevel1;

function test() {
  waitForExplicitFinish();

  Task.spawn(function () {
    gTab1 = gBrowser.addTab();
    gTab2 = gBrowser.addTab();

    yield FullZoomHelper.selectTabAndWaitForLocationChange(gTab1);
    yield FullZoomHelper.load(gTab1, TEST_PAGE);
    yield FullZoomHelper.load(gTab2, TEST_PAGE);
  }).then(zoomTab1, FullZoomHelper.failAndContinue(finish));
}

function dispatchZoomEventToBrowser(browser) {
  EventUtils.synthesizeWheel(browser.contentDocument.documentElement, 10, 10, {
    ctrlKey: true, deltaY: -1, deltaMode: WheelEvent.DOM_DELTA_LINE
  }, browser.contentWindow);
}

function zoomTab1() {
  Task.spawn(function () {
    is(gBrowser.selectedTab, gTab1, "Tab 1 is selected");
    FullZoomHelper.zoomTest(gTab1, 1, "Initial zoom of tab 1 should be 1");
    FullZoomHelper.zoomTest(gTab2, 1, "Initial zoom of tab 2 should be 1");

    let browser1 = gBrowser.getBrowserForTab(gTab1);
    dispatchZoomEventToBrowser(browser1);

    gLevel1 = ZoomManager.getZoomForBrowser(browser1);
    ok(gLevel1 > 1, "New zoom for tab 1 should be greater than 1");

    yield FullZoomHelper.selectTabAndWaitForLocationChange(gTab2);
    FullZoomHelper.zoomTest(gTab2, gLevel1, "Tab 2 should have zoomed along with tab 1");
  }).then(finishTest, FullZoomHelper.failAndContinue(finish));
}

function finishTest() {
  Task.spawn(function () {
    yield FullZoomHelper.selectTabAndWaitForLocationChange(gTab1);
    FullZoom.reset();
    yield FullZoomHelper.removeTabAndWaitForLocationChange(gTab1);
    yield FullZoomHelper.selectTabAndWaitForLocationChange(gTab2);
    FullZoom.reset();
    yield FullZoomHelper.removeTabAndWaitForLocationChange(gTab2);
  }).then(finish, FullZoomHelper.failAndContinue(finish));
}
