




































function test() {
  waitForExplicitFinish();
  ok(!gFindBarInitialized, "find bar is not yet initialized");

  let tab = gBrowser.addTab();
  gBrowser.selectedTab = tab;
  tab.linkedBrowser.addEventListener("load", function(aEvent) {
    tab.linkedBrowser.removeEventListener("load", arguments.callee, true);

    ok(true, "Load listener called");
    waitForFocus(onFocus, content);
  }, true);

  content.location = "data:text/html,<h1>A Page</h1>";
}

function onFocus() {
  EventUtils.synthesizeKey("/", {});
  ok(gFindBarInitialized, "find bar is now initialized");
  gFindBar.close();
  gBrowser.removeCurrentTab();
  finish();
}
