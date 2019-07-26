




"use strict";

function test() {
  runTests();
}

gTests.push({
  desc: "Access the find bar with the keyboard",
  run: function() {
    let tab = yield addTab(chromeRoot + "browser_findbar.html");
    yield waitForCondition(() => BrowserUI.ready);
    is(Elements.findbar.isShowing, false, "Find bar is hidden by default");

    EventUtils.synthesizeKey("f", { accelKey: true });
    yield waitForEvent(Elements.findbar, "transitionend");
    is(Elements.findbar.isShowing, true, "Show find bar with Ctrl-F");

    let textbox = document.getElementById("findbar-textbox");
    is(textbox.value, "", "Find bar is empty");

    EventUtils.sendString("bar");
    is(textbox.value, "bar", "Type 'bar' into find bar");

    EventUtils.synthesizeKey("VK_ESCAPE", { accelKey: true });
    yield waitForEvent(Elements.findbar, "transitionend");
    is(Elements.findbar.isShowing, false, "Hide find bar with Esc");

    Browser.closeTab(tab);
  }
});

gTests.push({
  desc: "Show and hide the find bar with mouse",
  run: function() {
    let tab = yield addTab(chromeRoot + "browser_findbar.html");
    yield waitForCondition(() => BrowserUI.ready);
    is(Elements.findbar.isShowing, false, "Find bar is hidden by default");

    yield showNavBar();
    EventUtils.sendMouseEvent({ type: "click" }, "menu-button");
    EventUtils.sendMouseEvent({ type: "click" }, "context-findinpage");
    yield waitForEvent(Elements.findbar, "transitionend");
    is(Elements.findbar.isShowing, true, "Show find bar with menu item");

    EventUtils.synthesizeMouse(document.getElementById("findbar-close"), 1, 1, {});
    yield waitForEvent(Elements.findbar, "transitionend");
    is(Elements.findbar.isShowing, false, "Hide find bar with close button");

    Browser.closeTab(tab);
  }
});
