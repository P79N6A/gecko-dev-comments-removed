









































const TEST_URI = "http://example.com/browser/toolkit/components/console/hudservice/tests/browser/test-console.html";

function test() {
  addTab(TEST_URI);
  browser.addEventListener("DOMContentLoaded", testStorageIteration, false);
}

function testStorageIteration() {
  browser.removeEventListener("DOMContentLoaded", testStorageIteration,
                              false);

  openConsole();

  let cs = HUDService.storage;

  
  cs.createDisplay("foo");
  for (let i = 0; i < 300; i++) {
    cs.recordEntry("foo", { logLevel: "network", message: "foo" });
  }

  var id = "foo";
  var it = cs.displayStore(id);
  var entry = it.next();
  var entry2 = it.next();

  let entries = [];
  for (var i = 0; i < 100; i++) {
    let _entry = it.next();
    entries.push(_entry);
  }

  ok(entries.length == 100, "entries length == 100");

  let entries2 = [];

  for (var i = 0; i < 100; i++){
    let _entry = it.next();
    entries2.push(_entry);
  }

  ok(entries[0].id != entries2[0].id,
     "two distinct pages of log entries");

  cs.removeDisplay("foo");
  cs = null;
  
  finishTest();
}

