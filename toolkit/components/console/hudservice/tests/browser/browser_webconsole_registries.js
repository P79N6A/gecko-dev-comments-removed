










































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
  var registry = HUDService.displayRegistry;
  var uri = registry[display];
  ok(registry[display], "we have a URI: " + registry[display]);

  var uriRegistry = HUDService.uriRegistry;
  ok(uriRegistry[uri].length == 1, "uri registry is working");

  finishTest();
}

