




"use strict";

gTests.push({
  desc: "about flyout hides navbar, clears navbar selection, doesn't leak",
  run: function() {
    yield showNavBar();

    let edit = document.getElementById("urlbar-edit");
    edit.value = "http://www.wikipedia.org/";

    sendElementTap(window, edit);

    yield waitForCondition(function () {
      return SelectionHelperUI.isSelectionUIVisible;
    });
    ok(ContextUI.navbarVisible, "nav bar visible");

    let promise = waitForEvent(FlyoutPanelsUI.AboutFlyoutPanel._topmostElement, "transitionend");
    FlyoutPanelsUI.show('AboutFlyoutPanel');
    yield promise;

    yield waitForCondition(function () {
      return !SelectionHelperUI.isSelectionUIVisible;
    });
    ok(!ContextUI.navbarVisible, "nav bar hidden");

    promise = waitForEvent(FlyoutPanelsUI.AboutFlyoutPanel._topmostElement, "transitionend");
    FlyoutPanelsUI.hide('AboutFlyoutPanel');
    yield promise;
  }
});

function test() {
  if (!isLandscapeMode()) {
    todo(false, "browser_selection_tests need landscape mode to run.");
    return;
  }
  runTests();
}
