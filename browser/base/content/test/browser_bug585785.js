var tab;

function test() {
  waitForExplicitFinish();

  tab = gBrowser.addTab();
  is(tab.getAttribute("fadein"), "true", "tab opening animation initiated");

  
  window.mozRequestAnimationFrame(checkAnimationState);
}

function checkAnimationState() {
  info(window.getComputedStyle(tab).maxWidth);
  gBrowser.removeTab(tab, { animate: true });
  if (!tab.parentNode) {
    ok(true, "tab removed synchronously since the opening animation hasn't moved yet");
    finish();
    return;
  }

  info("tab didn't close immediately, so the tab opening animation must have started moving");
  info("waiting for the tab to close asynchronously");
  tab.addEventListener("transitionend", function (event) {
    if (event.propertyName == "max-width") {
      tab.removeEventListener("transitionend", arguments.callee, false);
      executeSoon(function () {
        ok(!tab.parentNode, "tab removed asynchronously");
        finish();
      });
    }
  }, false);
}
