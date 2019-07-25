









































const TEST_URI = "http://example.com/browser/toolkit/components/console/hudservice/tests/browser/test-console.html";

function test() {
  addTab(TEST_URI);
  browser.addEventListener("DOMContentLoaded", testDisplayAccessors, false);
}

function testDisplayAccessors() {
  browser.removeEventListener("DOMContentLoaded", testDisplayAccessors,
                              false);

  openConsole();

  var allHuds = HUDService.displays();
  ok(typeof allHuds == "object", "allHuds is an object");
  var idx = HUDService.displaysIndex();

  let hudId = idx[0];

  ok(typeof idx == "object", "displays is an object");
  ok(typeof idx.push == "function", "displaysIndex is an array");

  var len = idx.length;
  ok(idx.length > 0, "idx.length > 0: " + len);

  finishTest();
}
