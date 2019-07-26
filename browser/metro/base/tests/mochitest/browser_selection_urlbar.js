




"use strict";

let gWindow = null;
var gFrame = null;

const kMarkerOffsetY = 12;
const kCommonWaitMs = 5000;
const kCommonPollMs = 100;





gTests.push({
  desc: "normalize browser",
  run: function test() {
    info(chromeRoot + "res/textblock01.html");
    yield addTab(chromeRoot + "res/textblock01.html");

    yield waitForCondition(function () {
      return !StartUI.isStartPageVisible;
      });

    yield hideContextUI();

    InputSourceHelper.isPrecise = false;
    InputSourceHelper.fireUpdate();
  },
});

gTests.push({
  desc: "nav bar display",
  run: function test() {
    gWindow = window;

    yield showNavBar();

    let edit = document.getElementById("urlbar-edit");

    sendElementTap(window, edit, 100, 10);

    ok(SelectionHelperUI.isSelectionUIVisible, "selection ui active");

    sendElementTap(window, edit, 70, 10);

    ok(SelectionHelperUI.isCaretUIVisible, "caret ui active");

    
    let xpos = SelectionHelperUI.caretMark.xPos;
    let ypos = SelectionHelperUI.caretMark.yPos + 10;
    var touchdrag = new TouchDragAndHold();
    yield touchdrag.start(gWindow, xpos, ypos, 900, ypos);
    yield waitForCondition(function () {
      return getTrimmedSelection(edit).toString() == 
        "mochitests/content/metro/browser/metro/base/tests/mochitest/res/textblock01.html";
    }, kCommonWaitMs, kCommonPollMs);
    touchdrag.end();
    yield waitForMs(100);

    ok(SelectionHelperUI.isSelectionUIVisible, "selection ui active");

    
    delete window.r;
  },
});

function test() {
  if (!isLandscapeMode()) {
    todo(false, "browser_selection_tests need landscape mode to run.");
    return;
  }
  runTests();
}
