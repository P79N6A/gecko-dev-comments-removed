










































const TEST_URI = "http://example.com/browser/toolkit/components/console/hudservice/tests/browser/test-console.html";

function test() {
  addTab(TEST_URI);
  browser.addEventListener("DOMContentLoaded", testRegistries, false);
}

function testRegistries() {
  browser.removeEventListener("DOMContentLoaded", testRegistries, false);

  openConsole();

  var displaysIdx = HUDService.displaysIndex();
  ok(displaysIdx.length == 1, "one display id found");

  var display = displaysIdx[0];
  ok(HUDService.hudReferences[display], "we have a HUD");

  let windowID = HUDService.getWindowId(content);
  is(HUDService.windowIds[windowID], display, "windowIds is working");

  finishTest();
}

