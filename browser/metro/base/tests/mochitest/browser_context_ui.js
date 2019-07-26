




"use strict";

function test() {
  runTests();
}

gTests.push({
  desc: "Context UI on about:start",
  run: function testAboutStart() {
    yield addTab("about:start");
    is(StartUI.isVisible, true, "Start UI is displayed on about:start");
    is(ContextUI.isVisible, true, "Toolbar is displayed on about:start");
    is(ContextUI.isExpanded, false, "Tab bar is not displayed initially");
    is(Elements.appbar.isShowing, false, "Appbar is not displayed initially");

    
    doEdgeUIGesture();
    is(ContextUI.isVisible, true, "Toolbar is still visible after one swipe");
    is(ContextUI.isExpanded, true, "Tab bar is visible after one swipe");
    is(Elements.appbar.isShowing, true, "Appbar is visible after one swipe");

    
    doEdgeUIGesture();
    is(ContextUI.isVisible, true, "Toolbar is still visible after second swipe");
    is(ContextUI.isExpanded, false, "Tab bar is hidden after second swipe");
    is(Elements.appbar.isShowing, false, "Appbar is hidden after second swipe");

    
    doEdgeUIGesture();
    is(ContextUI.isVisible, true, "Toolbar is still visible after third swipe");
    is(ContextUI.isExpanded, true, "Tab bar is visible after third swipe");
    is(Elements.appbar.isShowing, true, "Appbar is visible after third swipe");

    is(StartUI.isVisible, true, "Start UI is still visible");
  }
});

gTests.push({
  desc: "Context UI on a web page (about:)",
  run: function testAbout() {
    yield addTab("about:");
    ContextUI.dismiss();
    is(StartUI.isVisible, false, "Start UI is not visible on about:");
    is(ContextUI.isVisible, false, "Toolbar is not initially visible on about:");
    is(ContextUI.isExpanded, false, "Tab bar is not initially visible on about:");
    is(Elements.appbar.isShowing, false, "Appbar is not initially visible on about on about::");

    doEdgeUIGesture();
    is(ContextUI.isVisible, true, "Toolbar is visible after one swipe");
    is(ContextUI.isExpanded, true, "Tab bar is visble after one swipe");
    is(Elements.appbar.isShowing, true, "Appbar is visible after one swipe");

    doEdgeUIGesture();
    is(ContextUI.isVisible, false, "Toolbar is not visible after second swipe");
    is(ContextUI.isExpanded, false, "Tab bar is not visible after second swipe");
    is(Elements.appbar.isShowing, false, "Appbar is hidden after second swipe");

    is(StartUI.isVisible, false, "Start UI is still not visible");
  }
});

gTests.push({
  desc: "Control-L keyboard shortcut",
  run: function testAbout() {
    let tab = yield addTab("about:");
    ContextUI.dismiss();
    is(ContextUI.isVisible, false, "Navbar is not initially visible");
    is(ContextUI.isExpanded, false, "Tab bar is not initially visible");

    EventUtils.synthesizeKey('l', { accelKey: true });
    is(ContextUI.isVisible, true, "Navbar is visible");
    is(ContextUI.isExpanded, false, "Tab bar is not visible");

    let edit = document.getElementById("urlbar-edit");
    is(edit.value, "about:", "Location field contains the page URL");
    ok(document.commandDispatcher.focusedElement, edit.inputField, "Location field is focused");
    is(edit.selectionStart, 0, "Location field is selected");
    is(edit.selectionEnd, edit.value.length, "Location field is selected");

    Browser.closeTab(tab, { forceClose: true });
  }
});

function doEdgeUIGesture() {
  let event = document.createEvent("Events");
  event.initEvent("MozEdgeUICompleted", true, false);
  window.dispatchEvent(event);
}
