









































const TEST_URI = "http://example.com/browser/toolkit/components/console/hudservice/tests/browser/test-console.html";

function test() {
  addTab(TEST_URI);
  browser.addEventListener("DOMContentLoaded", testGetHeadsUpDisplay,
                           false);
}

function testGetHeadsUpDisplay() {
  browser.removeEventListener("DOMContentLoaded", testGetHeadsUpDisplay,
                              false);
  openConsole();
  hudId = HUDService.displaysIndex()[0];
  hud = HUDService.getHeadsUpDisplay(hudId);
  ok(hud.getAttribute("id") == hudId, "found HUD node by Id.");
  finishTest();
}

