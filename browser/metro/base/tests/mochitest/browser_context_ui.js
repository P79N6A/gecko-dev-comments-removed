




"use strict";

function test() {
  runTests();
}

gTests.push({
  desc: "Context UI on about:start",
  run: function testAboutStart() {
    yield addTab("about:start");

    yield waitForCondition(function () {
      return BrowserUI.isStartTabVisible;
      });

    is(BrowserUI.isStartTabVisible, true, "Start UI is displayed on about:start");
    is(ContextUI.navbarVisible, true, "Navbar is displayed on about:start");
    is(ContextUI.tabbarVisible, false, "Tabbar is not displayed initially");
    is(ContextUI.contextAppbarVisible, false, "Appbar is not displayed initially");

    
    doEdgeUIGesture();
    is(ContextUI.navbarVisible, true, "Navbar is still visible after one swipe");
    is(ContextUI.tabbarVisible, true, "Tabbar is visible after one swipe");
    is(ContextUI.contextAppbarVisible, false, "Appbar is hidden after one swipe");

    
    doEdgeUIGesture();
    is(ContextUI.navbarVisible, true, "Navbar is still visible after second swipe");
    is(ContextUI.tabbarVisible, false, "Tabbar is hidden after second swipe");
    is(ContextUI.contextAppbarVisible, false, "Appbar is hidden after second swipe");

    
    doEdgeUIGesture();
    is(ContextUI.navbarVisible, true, "Navbar is still visible after third swipe");
    is(ContextUI.tabbarVisible, true, "Tabbar is visible after third swipe");
    is(ContextUI.contextAppbarVisible, false, "Appbar is hidden after third swipe");

    is(BrowserUI.isStartTabVisible, true, "Start UI is still visible");
  }
});

gTests.push({
  desc: "Context UI on a web page (about:)",
  run: function testAbout() {
    yield addTab("about:");
    ContextUI.dismiss();
    is(BrowserUI.isStartTabVisible, false, "Start UI is not visible on about:");
    is(ContextUI.navbarVisible, false, "Navbar is not initially visible on about:");
    is(ContextUI.tabbarVisible, false, "Tabbar is not initially visible on about:");

    doEdgeUIGesture();
    is(ContextUI.navbarVisible, true, "Navbar is visible after one swipe");
    is(ContextUI.tabbarVisible, true, "Tabbar is visble after one swipe");

    doEdgeUIGesture();
    is(ContextUI.navbarVisible, false, "Navbar is not visible after second swipe");
    is(ContextUI.tabbarVisible, false, "Tabbar is not visible after second swipe");

    is(BrowserUI.isStartTabVisible, false, "Start UI is still not visible");
  }
});

gTests.push({
  desc: "Control-L keyboard shortcut",
  run: function testAbout() {
    let tab = yield addTab("about:");
    ContextUI.dismiss();
    is(ContextUI.navbarVisible, false, "Navbar is not initially visible");
    is(ContextUI.tabbarVisible, false, "Tab bar is not initially visible");

    EventUtils.synthesizeKey('l', { accelKey: true });
    is(ContextUI.navbarVisible, true, "Navbar is visible");
    is(ContextUI.tabbarVisible, false, "Tab bar is not visible");

    let edit = document.getElementById("urlbar-edit");
    is(edit.value, "about:", "Location field contains the page URL");
    ok(document.commandDispatcher.focusedElement, edit.inputField, "Location field is focused");
    is(edit.selectionStart, 0, "Location field is selected");
    is(edit.selectionEnd, edit.value.length, "Location field is selected");

    edit.selectionEnd = 0;
    is(edit.selectionStart, 0, "Location field is unselected");
    is(edit.selectionEnd, 0, "Location field is unselected");

    EventUtils.synthesizeKey('l', { accelKey: true });
    is(edit.selectionStart, 0, "Location field is selected again");
    is(edit.selectionEnd, edit.value.length, "Location field is selected again");

    Browser.closeTab(tab, { forceClose: true });
  }
});

function doEdgeUIGesture() {
  let event = document.createEvent("Events");
  event.initEvent("MozEdgeUICompleted", true, false);
  window.dispatchEvent(event);
}
