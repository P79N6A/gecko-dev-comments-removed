



"use strict";




Components.utils.import("resource://services-sync/main.js");



function test() {
  runTests();
}

gTests.push({
  desc: "Test sync tabs from other devices UI",
  run: function run() {
    if (StartUI.isStartPageVisible)
      return;

    yield addTab("about:start");
    yield waitForCondition(() => StartUI.isStartPageVisible);
    yield hideContextUI();

    is(Weave.Status.checkSetup(), Weave.CLIENT_NOT_CONFIGURED, "Sync should be disabled on start");

    let vbox = document.getElementById("start-remotetabs");
    ok(vbox.hidden, "remote tabs in the start page should be hidden when sync is not enabled");

    
    let menulink = document.getElementById("menuitem-remotetabs");
    ok(menulink.hidden, "link to container should be hidden when sync is not enabled");

    RemoteTabsStartView._view.setUIAccessVisible(true);

    
    ok(vbox, "remote tabs grid is present on start page");
    is(vbox.hidden, false, "remote tabs should be visible in start page when sync is enabled");

    RemoteTabsStartView._view.setUIAccessVisible(false);

    ok(vbox.hidden, "remote tabs in the start page should be hidden when sync is not enabled");
    ok(menulink.hidden, "link to container should be hidden when sync is not enabled");
  }
});
