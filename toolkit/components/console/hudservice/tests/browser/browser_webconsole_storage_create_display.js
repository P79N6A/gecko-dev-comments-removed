









































const TEST_URI = "http://example.com/browser/toolkit/components/console/hudservice/tests/browser/test-console.html";

function test() {
  addTab(TEST_URI);
  browser.addEventListener("DOMContentLoaded", testStorageCreateDisplay,
                           false);
}

function testStorageCreateDisplay() {
  browser.removeEventListener("DOMContentLoaded", testStorageCreateDisplay,
                              false);

  openConsole();

  let cs = HUDService.storage;

  ok(typeof cs.consoleDisplays == "object",
     "consoledisplays exist");
  ok(typeof cs.displayIndexes == "object",
     "console indexes exist");
  cs.createDisplay("foo");
  ok(typeof cs.consoleDisplays["foo"] == "object",
     "foo display exists");
  ok(typeof cs.displayIndexes["foo"] == "object",
     "foo index exists");

  cs.removeDisplay("foo");

  finishTest();
}

