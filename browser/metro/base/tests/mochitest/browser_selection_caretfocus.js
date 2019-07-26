




"use strict";

let gWindow = null;





function setUpAndTearDown() {
  emptyClipboard();
  if (gWindow)
    clearSelection(gWindow);
  yield waitForCondition(function () {
    return !SelectionHelperUI.isSelectionUIVisible;
  });
  InputSourceHelper.isPrecise = false;
  InputSourceHelper.fireUpdate();
}

gTests.push({
  desc: "normalize browser",
  setUp: setUpAndTearDown,
  tearDown: setUpAndTearDown,
  run: function test() {
    info(chromeRoot + "browser_selection_caretfocus.html");
    yield addTab(chromeRoot + "browser_selection_caretfocus.html");

    yield waitForCondition(function () {
      return !StartUI.isStartPageVisible;
    });

    yield hideContextUI();

    gWindow = Browser.selectedTab.browser.contentWindow;
  },
});

function tapText(aIndex) {
  gWindow = Browser.selectedTab.browser.contentWindow;
  let id = "Text" + aIndex;
  info("tapping " + id);
  let element = gWindow.document.getElementById(id);
  if (element.contentDocument) {
    element = element.contentDocument.getElementById("textarea");
    gWindow = element.ownerDocument.defaultView;
  }
  sendElementTap(gWindow, element, 100, 10);
  return element;
}

gTests.push({
  desc: "focus navigation",
  setUp: setUpAndTearDown,
  tearDown: setUpAndTearDown,
  run: function test() {
    for (let iteration = 0; iteration < 3; iteration++) {
      for (let input = 1; input <= 6; input++) {
        let element = tapText(input);
        if (input == 6) {
          
          yield waitForCondition(function () {
            return !SelectionHelperUI.isActive;
          });
        } else {
          
          yield SelectionHelperUI.pingSelectionHandler();
          yield waitForCondition(function () {
            return SelectionHelperUI.isCaretUIVisible;
          });
          ok(element == gWindow.document.activeElement, "element has focus");
        }
      }
    }
  },
});

function test() {
  if (!isLandscapeMode()) {
    todo(false, "browser_selection_tests need landscape mode to run.");
    return;
  }
  
  setDevPixelEqualToPx();
  runTests();
}
