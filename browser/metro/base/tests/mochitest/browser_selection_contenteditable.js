




"use strict";
let gWindow = null;

const kCommonWaitMs = 5000;
const kCommonPollMs = 100;





gTests.push({
  desc: "normalize browser",
  run: function test() {
    info(chromeRoot + "browser_selection_contenteditable.html");
    yield addTab(chromeRoot + "browser_selection_contenteditable.html");
    yield waitForCondition(function () {
        return !BrowserUI.isStartTabVisible;
      }, kCommonWaitMs, kCommonPollMs);

    yield hideContextUI();

    gWindow = Browser.selectedTab.browser.contentWindow;

    InputSourceHelper.isPrecise = false;
    InputSourceHelper.fireUpdate();
  },
});

gTests.push({
  desc: "simple test to make sure content editable selection works",
  run: function test() {
    let div = gWindow.document.getElementById("testdiv");
    ok(div, "have the div");

    sendElementTap(gWindow, div, 287); 

    yield waitForCondition(function () {
        return SelectionHelperUI.isCaretUIVisible;
      }, kCommonWaitMs, kCommonPollMs);

    let xpos = SelectionHelperUI.caretMark.xPos;
    let ypos = SelectionHelperUI.caretMark.yPos + 10;
    var touchdrag = new TouchDragAndHold();
    yield touchdrag.start(gWindow, xpos, ypos, 100, ypos); 
    touchdrag.end();
    yield waitForCondition(function () {
        return !SelectionHelperUI.hasActiveDrag;
      }, kCommonWaitMs, kCommonPollMs);
    yield SelectionHelperUI.pingSelectionHandler();
    let str = getTrimmedSelection(gWindow).toString();
    is(str, "sending an email in the outlook.com", "selected string matches");
  },
  tearDown: function () {
    emptyClipboard();
    if (gWindow)
      clearSelection(gWindow);
    yield waitForCondition(function () {
      return !SelectionHelperUI.isSelectionUIVisible;
    }, kCommonWaitMs, kCommonPollMs);
  }
});

function test() {
  if (!isLandscapeMode()) {
    todo(false, "browser_selection_tests need landscape mode to run.");
    return;
  }
  setDevPixelEqualToPx();
  runTests();
}
