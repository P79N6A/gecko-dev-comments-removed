



"use strict";

function test() {
  if (!isLandscapeMode()) {
    todo(false, "browser_snapped_tests need landscape mode to run.");
    return;
  }

  runTests();
}
let tabAdded = false;

function setUp() {
  if (!tabAdded) {
    yield addTab(chromeRoot + "res/textdivs01.html");
    tabAdded = true;
  }
  yield hideContextUI();
}

XPCOMUtils.defineLazyServiceGetter(this, "gDOMUtils",
  "@mozilla.org/inspector/dom-utils;1", "inIDOMUtils");

const kActiveState = 0x00000001;
const kHoverState = 0x00000004;

gTests.push({
  desc: "hover states of menus",
  setUp: setUp,
  run: function() {
    
    

    let typesArray = [
      "copy",
      "paste"
    ];

    let promise = waitForEvent(document, "popupshown");
    ContextMenuUI.showContextMenu({
      target: null,
      json: {
        types: typesArray,
        string: '',
        xPos: 1,
        yPos: 1,
        leftAligned: true,
        bottomAligned: true
    }});
    yield promise;

    
    ok(ContextMenuUI._menuPopup.visible, "is visible");

    let menuItem = document.getElementById("context-copy");
    promise = waitForEvent(document, "popuphidden");
    sendNativeTap(menuItem);
    yield promise;

    for (let idx = 0; idx < ContextMenuUI.commands.childNodes.length; idx++) {
      let item = ContextMenuUI.commands.childNodes[idx];
      let state = gDOMUtils.getContentState(item);
      if ((state & kHoverState) || (state & kActiveState)) {
        ok(false, "found invalid state on context menu item (" + state.toString(2) + ")");
      }
    }

    
    
    promise = waitForEvent(document, "popupshown");
    ContextMenuUI.showContextMenu({
      target: null,
      json: {
        types: typesArray,
        string: '',
        xPos: 1,
        yPos: 1,
        leftAligned: true,
        bottomAligned: true
    }});
    yield promise;

    
    ok(ContextMenuUI._menuPopup.visible, "is visible");

    for (let idx = 0; idx < ContextMenuUI.commands.childNodes.length; idx++) {
      let item = ContextMenuUI.commands.childNodes[idx];
      let state = gDOMUtils.getContentState(item);
      if ((state & kHoverState) || (state & kActiveState)) {
        ok(false, "found invalid state on context menu item (" + state.toString(2) + ")");
      }
    }

    menuItem = document.getElementById("context-paste");
    promise = waitForEvent(document, "popuphidden");
    sendNativeTap(menuItem);
    yield promise;

    for (let idx = 0; idx < ContextMenuUI.commands.childNodes.length; idx++) {
      let item = ContextMenuUI.commands.childNodes[idx];
      let state = gDOMUtils.getContentState(item);
      if ((state & kHoverState) || (state & kActiveState)) {
        ok(false, "found invalid state on context menu item (" + state.toString(2) + ")");
      }
    }
  },
  tearDown: function () {
    clearNativeTouchSequence();
  }
});

gTests.push({
  desc: "hover states of nav bar buttons",
  setUp: setUp,
  run: function() {
    
    yield showNavBar();

    
    sendNativeTap(Appbar.starButton);
    yield waitForMs(100);
    
    
    let state = gDOMUtils.getContentState(Appbar.starButton);
    if ((state & kHoverState) || (state & kActiveState)) {
      ok(false, "found invalid state on star button (" + state.toString(2) + ")");
    }

    
    sendNativeTap(Appbar.starButton);
    yield waitForMs(100);
    
    
    let state = gDOMUtils.getContentState(Appbar.starButton);
    if ((state & kHoverState) || (state & kActiveState)) {
      ok(false, "found invalid state on star button (" + state.toString(2) + ")");
    }
  },
  tearDown: function () {
    clearNativeTouchSequence();
  }
});

