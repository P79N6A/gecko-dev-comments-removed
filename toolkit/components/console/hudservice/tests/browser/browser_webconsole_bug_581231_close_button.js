











Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://gre/modules/HUDService.jsm");

const TEST_URI = "http://example.com/browser/toolkit/components/console/hudservice/tests/browser/test-console.html";

function test() {
  waitForExplicitFinish();
  content.location.href = TEST_URI;
  waitForFocus(onFocus);
}

function onFocus() {
  let tabBrowser = gBrowser.getBrowserForTab(gBrowser.selectedTab);
  tabBrowser.addEventListener("DOMContentLoaded", testCloseButton, false);
}

function testCloseButton() {
  let tabBrowser = gBrowser.getBrowserForTab(gBrowser.selectedTab);
  tabBrowser.removeEventListener("DOMContentLoaded", testCloseButton, false);

  HUDService.activateHUDForContext(gBrowser.selectedTab);

  let hudId = HUDService.displaysIndex()[0];
  let hudBox = HUDService.getHeadsUpDisplay(hudId);

  let closeButton = hudBox.querySelector(".jsterm-close-button");
  ok(closeButton != null, "we have the close button");

  EventUtils.synthesizeMouse(closeButton, 0, 0, {});

  ok(!(hudId in HUDService.windowRegistry), "the console is closed when the " +
     "close button is pressed");

  finish();
}

