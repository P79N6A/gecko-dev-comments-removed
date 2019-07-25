









































const TEST_URI = "http://example.com/browser/toolkit/components/console/hudservice/tests/browser/test-console.html";

function test() {
  addTab(TEST_URI);
  browser.addEventListener("DOMContentLoaded", testGetDisplayByURISpec,
                           false);
}

function testGetDisplayByURISpec() {
  browser.removeEventListener("DOMContentLoaded", testGetDisplayByURISpec,
                              false);
  openConsole();
  outputNode = HUDService.getDisplayByURISpec(TEST_URI);
  hudId = outputNode.getAttribute("id");
  ok(hudId == HUDService.displaysIndex()[0], "outputNode fetched by URIspec");
  finishTest();
}

