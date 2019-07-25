




































function test() {
  waitForExplicitFinish();
  ok(!gFindBarInitialized, "find bar is not yet initialized");

  gBrowser.selectedBrowser.addEventListener("pageshow", function () {
    ok(true, "Load listener called");
    gBrowser.selectedBrowser.removeEventListener("pageshow", arguments.callee, false);

    waitForFocus(onFocus, content);
  }, false);

  gBrowser.loadURI("data:text/html,<h1>A Page</h1>");
}

function onFocus() {
  ok(!gFindBarInitialized, "find bar is not yet initialized");
  EventUtils.synthesizeKey("/", {});
  ok(gFindBarInitialized, "find bar is now initialized");

  gBrowser.addTab();
  gBrowser.removeCurrentTab();
  finish();
}
