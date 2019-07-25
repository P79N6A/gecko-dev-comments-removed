









































const TEST_URI = "http://example.com/browser/toolkit/components/console/hudservice/tests/browser/test-console.html";

function test() {
  addTab(TEST_URI);
  browser.addEventListener("DOMContentLoaded",
                           testGetContentWindowFromHUDId, false);
}

function testGetContentWindowFromHUDId() {
  browser.removeEventListener("DOMContentLoaded",
                              testGetContentWindowFromHUDId, false);

  openConsole();

  let hudId = HUDService.displaysIndex()[0];

  let window = HUDService.getContentWindowFromHUDId(hudId);
  ok(window.document, "we have a contentWindow");

  finishTest();
}

