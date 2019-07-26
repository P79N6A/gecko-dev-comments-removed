



"use strict";

function test() {
  if (!isLandscapeMode()) {
    todo(false, "browser_snapped_tests need landscape mode to run.");
    return;
  }

  runTests();
}

let kTransformTimeout = 5000;

let gEdit = null;
let tabAdded = false;

function setUp() {
  if (!tabAdded) {
    yield addTab(chromeRoot + "res/textdivs01.html");
    tabAdded = true;
  }
  yield hideContextUI();
}





























gTests.push({
  desc: "native long tap works",
  setUp: setUp,
  run: function() {
    let edit = Browser.selectedBrowser.contentDocument.getElementById("textinput");
    let promise = waitForEvent(document, "popupshown");
    sendNativeLongTap(edit);
    yield promise;
    ContextMenuUI.hide();
  },
  tearDown: function () {
    clearNativeTouchSequence();
  }
});




























gTests.push({
  desc: "scroll transforms",
  setUp: setUp,
  run: function() {
    let beginPromise = waitForObserver("apzc-transform-begin", kTransformTimeout);
    let endPromise = waitForObserver("apzc-transform-end", kTransformTimeout);

    var touchdrag = new TouchDragAndHold();
    touchdrag.useNativeEvents = true;
    touchdrag.nativePointerId = 1;
    yield touchdrag.start(Browser.selectedTab.browser.contentWindow,
                          10, 100, 10, 10);
    touchdrag.end();

    yield beginPromise;
    yield endPromise;
  },
  tearDown: function () {
    clearNativeTouchSequence();
  }
});
