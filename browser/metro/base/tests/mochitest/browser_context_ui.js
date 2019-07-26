




"use strict";

function test() {
  runTests();
}

gTests.push({
  desc: "Context UI on about:start",
  run: function testAboutStart() {
    let tab = yield addTab("about:start");

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

    Browser.closeTab(tab, { forceClose: true });
  }
});

gTests.push({
  desc: "Context UI on a web page (about:)",
  run: function testAbout() {
    let tab = yield addTab("about:");
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

    Browser.closeTab(tab, { forceClose: true });
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

function getpage(idx) {
  return "http://mochi.test:8888/metro/browser/metro/base/tests/mochitest/" + "res/blankpage" + idx + ".html";
}

gTests.push({
  desc: "taps vs context ui dismissal",
  run: function () {
    
    InputSourceHelper.isPrecise = false;
    InputSourceHelper.fireUpdate();

    let tab = yield addTab("about:mozilla");

    ok(ContextUI.navbarVisible, "navbar visible after open");

    let navButtonDisplayPromise = waitForEvent(NavButtonSlider.back, "transitionend");

    yield loadUriInActiveTab(getpage(1));

    is(tab.browser.currentURI.spec, getpage(1), getpage(1));
    ok(ContextUI.navbarVisible, "navbar visible after navigate 1");

    yield loadUriInActiveTab(getpage(2));

    is(tab.browser.currentURI.spec, getpage(2), getpage(2));
    ok(ContextUI.navbarVisible, "navbar visible after navigate 2");

    yield loadUriInActiveTab(getpage(3));

    is(tab.browser.currentURI.spec, getpage(3), getpage(3));
    ok(ContextUI.navbarVisible, "navbar visible after navigate 3");

    
    
    
    yield navButtonDisplayPromise;

    yield navBackViaNavButton();
    yield waitForCondition2(function () { return tab.browser.currentURI.spec == getpage(2); }, "getpage(2)");
    yield waitForCondition2(function () { return ContextUI.navbarVisible; }, "ContextUI.navbarVisible");

    is(tab.browser.currentURI.spec, getpage(2), getpage(2));

    yield navForward();
    yield waitForCondition2(function () { return tab.browser.currentURI.spec == getpage(3); }, "getpage(3)");

    is(tab.browser.currentURI.spec, getpage(3), getpage(3));
    ok(ContextUI.navbarVisible, "navbar visible after navigate");

    doEdgeUIGesture();

    is(ContextUI.navbarVisible, true, "Navbar is visible after swipe");
    is(ContextUI.tabbarVisible, true, "Tabbar is visible after swipe");

    yield navBackViaNavButton();
    yield waitForCondition2(function () { return tab.browser.currentURI.spec == getpage(2); }, "getpage(2)");

    is(tab.browser.currentURI.spec, getpage(2), getpage(2));
    is(ContextUI.navbarVisible, true, "Navbar is visible after navigating back (overlay)");
    yield waitForCondition2(function () { return !ContextUI.tabbarVisible; }, "!ContextUI.tabbarVisible");

    sendElementTap(window, window.document.documentElement);
    yield waitForCondition2(function () { return !BrowserUI.navbarVisible; }, "!BrowserUI.navbarVisible");

    is(ContextUI.tabbarVisible, false, "Tabbar is hidden after content tap");

    yield navForward();
    yield waitForCondition2(function () { return tab.browser.currentURI.spec == getpage(3); }, "getpage(3)");

    is(tab.browser.currentURI.spec, getpage(3), getpage(3));
    ok(ContextUI.navbarVisible, "navbar visible after navigate");

    yield navBackViaNavButton();
    yield waitForCondition2(function () { return tab.browser.currentURI.spec == getpage(2); }, "getpage(2)");
    yield waitForCondition2(function () { return !ContextUI.tabbarVisible; }, "!ContextUI.tabbarVisible");

    is(tab.browser.currentURI.spec, getpage(2), getpage(2));
    is(ContextUI.navbarVisible, true, "Navbar is visible after navigating back (overlay)");

    ContextUI.dismiss();

    let note = yield showNotification();
    doEdgeUIGesture();
    sendElementTap(window, note);

    is(ContextUI.navbarVisible, true, "Navbar is visible after clicking notification close button");

    removeNotifications();

    Browser.closeTab(tab, { forceClose: true });
  }
});

function doEdgeUIGesture() {
  let event = document.createEvent("Events");
  event.initEvent("MozEdgeUICompleted", true, false);
  window.dispatchEvent(event);
}
