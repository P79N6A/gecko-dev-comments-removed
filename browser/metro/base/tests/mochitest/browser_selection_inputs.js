




"use strict";

let gWindow = null;
var gInput = null;

const kCommonWaitMs = 7000;
const kCommonPollMs = 200;





function setUpAndTearDown() {
  emptyClipboard();
  if (gWindow)
    clearSelection(gWindow);
  if (gInput)
    clearSelection(gInput);
  yield waitForCondition(function () {
      return !SelectionHelperUI.isSelectionUIVisible;
    }, kCommonWaitMs, kCommonPollMs);
  InputSourceHelper.isPrecise = false;
  InputSourceHelper.fireUpdate();
}







gTests.push({
  desc: "normalize browser",
  setUp: setUpAndTearDown,
  tearDown: setUpAndTearDown,
  run: function test() {
    info(chromeRoot + "browser_selection_inputs.html");
    yield addTab(chromeRoot + "browser_selection_inputs.html");

    yield waitForCondition(function () {
      return !BrowserUI.isStartTabVisible;
      });

    yield hideContextUI();

    gWindow = Browser.selectedTab.browser.contentWindow;
    gInput = gWindow.document.getElementById("a");
  },
});

gTests.push({
  desc: "basic text input selection",
  setUp: setUpAndTearDown,
  tearDown: setUpAndTearDown,
  run: function test() {
    gInput.blur();
    gInput.selectionStart = gInput.selectionEnd = 0;

    let promise = waitForEvent(document, "popupshown");
    sendContextMenuClick(200, 17);
    yield promise;

    checkContextUIMenuItemVisibility(["context-select",
                                      "context-select-all"]);

    let menuItem = document.getElementById("context-select");
    ok(menuItem, "menu item exists");
    ok(!menuItem.hidden, "menu item visible");
    let popupPromise = waitForEvent(document, "popuphidden");
    sendElementTap(gWindow, menuItem);
    yield popupPromise;

    yield waitForCondition(function () {
        return SelectionHelperUI.isSelectionUIVisible;
      }, kCommonWaitMs, kCommonPollMs);

    is(getTrimmedSelection(gInput).toString(), "went", "selection test");
    is(gWindow.document.activeElement, gInput, "input focused");
  },
});

gTests.push({
  desc: "drag left to scroll",
  setUp: setUpAndTearDown,
  tearDown: setUpAndTearDown,
  run: function test() {
    gInput.selectionStart = gInput.selectionEnd = gInput.value.length;
    yield waitForEvent(window, "scroll");

    let promise = waitForEvent(document, "popupshown");
    sendContextMenuClick(190, 17);
    yield promise;

    checkContextUIMenuItemVisibility(["context-select",
                                      "context-select-all"]);

    let menuItem = document.getElementById("context-select");
    ok(menuItem, "menu item exists");
    ok(!menuItem.hidden, "menu item visible");
    let popupPromise = waitForEvent(document, "popuphidden");
    sendElementTap(gWindow, menuItem);
    yield popupPromise;

    yield waitForCondition(function () {
        return SelectionHelperUI.isSelectionUIVisible;
      }, kCommonWaitMs, kCommonPollMs);

    
    is(getTrimmedSelection(gInput).toString(), "way", "selection test");

    
    let xpos = SelectionHelperUI.startMark.xPos;
    let ypos = SelectionHelperUI.startMark.yPos + 10;
    var touchdrag = new TouchDragAndHold();
    yield touchdrag.start(gWindow, xpos, ypos, 10, ypos);
    yield waitForCondition(function () {
      return getTrimmedSelection(gInput).toString() == 
        "The rabbit-hole went straight on like a tunnel for some way";
    }, kCommonWaitMs, kCommonPollMs);
    touchdrag.end();

    yield waitForCondition(function () {
        return !SelectionHelperUI.hasActiveDrag;
      }, kCommonWaitMs, kCommonPollMs);
    yield SelectionHelperUI.pingSelectionHandler();
  },
});

gTests.push({
  desc: "drag right to scroll and bug 862025",
  setUp: setUpAndTearDown,
  tearDown: setUpAndTearDown,
  run: function test() {
    gInput.selectionStart = gInput.selectionEnd = 0;
    yield waitForEvent(window, "scroll");

    let promise = waitForEvent(document, "popupshown");
    sendContextMenuClick(230, 17);
    yield promise;

    checkContextUIMenuItemVisibility(["context-select",
                                      "context-select-all"]);

    let menuItem = document.getElementById("context-select");
    ok(menuItem, "menu item exists");
    ok(!menuItem.hidden, "menu item visible");
    let popupPromise = waitForEvent(document, "popuphidden");
    sendElementTap(gWindow, menuItem);
    yield popupPromise;

    yield waitForCondition(function () {
        return SelectionHelperUI.isSelectionUIVisible;
      }, kCommonWaitMs, kCommonPollMs);

    
    is(getTrimmedSelection(gInput).toString(), "straight", "selection test");

    
    let xpos = SelectionHelperUI.endMark.xPos;
    let ystartpos = SelectionHelperUI.endMark.yPos + 10;
    var touchdrag = new TouchDragAndHold();
    yield touchdrag.start(gWindow, xpos, ystartpos, 510, ystartpos);
    
    yield waitForCondition(function () {
      return getTrimmedSelection(gInput).toString() == 
        "straight on like a tunnel for some way and then dipped suddenly down";
    }, kCommonWaitMs, kCommonPollMs);
    touchdrag.end();

    yield waitForCondition(function () {
        return !SelectionHelperUI.hasActiveDrag;
      }, kCommonWaitMs, kCommonPollMs);
    yield SelectionHelperUI.pingSelectionHandler();

    
    let xpos = SelectionHelperUI.endMark.xPos;
    let ypos = SelectionHelperUI.endMark.yPos + 10;
    yield touchdrag.start(gWindow, xpos, ypos, xpos, ypos + 150);
    

    yield SelectionHelperUI.pingSelectionHandler();
    is(getTrimmedSelection(gInput).toString(), "straight on like a tunnel for some way and then dipped suddenly down", "selection test");

    
    yield touchdrag.move(135, ystartpos);
    touchdrag.end();

    yield SelectionHelperUI.pingSelectionHandler();
    is(getTrimmedSelection(gInput).toString(), "straight on like a tunnel for", "selection test");
  },
});

gTests.push({
  desc: "Bug 858206 - Drag selection monocles should not push other monocles " +
        "out of the way.",
  setUp: setUpAndTearDown,
  tearDown: setUpAndTearDown,
  run: function test() {
    let inputOriginalValue = gInput.value;

    gInput.value = "The rabbit-hole went straight on";

    let promise = waitForEvent(document, "popupshown");
    sendContextMenuClickToElement(gWindow, gInput, 150);
    yield promise;

    
    promise = waitForEvent(document, "popuphidden");
    sendElementTap(gWindow, document.getElementById("context-select"));
    yield promise;

    yield waitForCondition(() => SelectionHelperUI.isSelectionUIVisible,
        kCommonWaitMs, kCommonPollMs);
    is(getTrimmedSelection(gInput).toString(), "straight");

    
    let startXPos = SelectionHelperUI.endMark.xPos;
    let startYPos = SelectionHelperUI.endMark.yPos + 10;
    let touchDrag = new TouchDragAndHold();
    yield touchDrag.start(gWindow, startXPos, startYPos, startXPos - 300,
        startYPos);

    yield waitForCondition(() => getTrimmedSelection(gInput).toString() ==
        "The rabbit-hole went", kCommonWaitMs, kCommonPollMs);
    touchDrag.end();
    yield waitForCondition(() => !SelectionHelperUI.hasActiveDrag,
        kCommonWaitMs, kCommonPollMs);
    yield SelectionHelperUI.pingSelectionHandler();

     
    startXPos = SelectionHelperUI.startMark.xPos;
    startYPos = SelectionHelperUI.startMark.yPos + 10;
    yield touchDrag.start(gWindow, startXPos, startYPos, startXPos + 300,
        startYPos);
    yield waitForCondition(() => getTrimmedSelection(gInput).toString() ==
        "straight on", kCommonWaitMs, kCommonPollMs);
    touchDrag.end();
    yield waitForCondition(() => !SelectionHelperUI.hasActiveDrag,
        kCommonWaitMs, kCommonPollMs);
    yield SelectionHelperUI.pingSelectionHandler();

    
    gInput.selectionStart = gInput.selectionEnd = 0;
    sendElementTap(gWindow, gInput, 0, 0);

    yield waitForCondition(() => !SelectionHelperUI.isSelectionUIVisible &&
        SelectionHelperUI.isCaretUIVisible);

    startXPos = SelectionHelperUI.caretMark.xPos;
    startYPos = SelectionHelperUI.caretMark.yPos + 10;

    yield touchDrag.start(gWindow, startXPos, startYPos, startXPos + 300,
        startYPos);
    yield waitForCondition(() => getTrimmedSelection(gInput).toString() ==
        "The rabbit-hole went straight on", kCommonWaitMs, kCommonPollMs);
    touchDrag.end();

    sendTap(gWindow, 10, 10);
    yield waitForCondition(() => !SelectionHelperUI.isSelectionUIVisible);

    
    gInput.selectionStart = gInput.selectionEnd = gInput.value.length;
    let inputSelectionRectangle = gInput.QueryInterface(Ci.nsIDOMNSEditableElement).
        editor.selection.getRangeAt(0).getClientRects()[0];
    sendTap(gWindow, inputSelectionRectangle.right,
        inputSelectionRectangle.top);

    yield waitForCondition(() => SelectionHelperUI.isCaretUIVisible);

    startXPos = SelectionHelperUI.caretMark.xPos;
    startYPos = SelectionHelperUI.caretMark.yPos + 10;

    yield touchDrag.start(gWindow, startXPos, startYPos, startXPos - 300,
        startYPos);
    yield waitForCondition(() => getTrimmedSelection(gInput).toString() ==
        "The rabbit-hole went straight on", kCommonWaitMs, kCommonPollMs);
    touchDrag.end();

    gInput.value = inputOriginalValue;
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
