









































const TEST_URI = "http://example.com/browser/toolkit/components/console/hudservice/tests/browser/test-console.html";

function test() {
  addTab(TEST_URI);
  browser.addEventListener("DOMContentLoaded", testStorageRecordEntry,
                              false);
}

function testStorageRecordEntry() {
  browser.removeEventListener("DOMContentLoaded", testStorageRecordEntry,
                              false);
  openConsole();

  let cs = HUDService.storage;

  cs.createDisplay("foo");

  var config = {
    logLevel: "network",
    message: "HumminaHummina!",
    activity: {
      stage: "barStage",
      data: "bar bar bar bar"
    }
  };
  var entry = cs.recordEntry("foo", config);
  var res = entry.id;
  ok(entry.id != null, "Entry.id is: " + res);
  ok(cs.displayIndexes["foo"].length == 1,
     "We added one entry.");
  entry = cs.getEntry(res);
  ok(entry.id > -1,
     "We got an entry through the global interface");

  cs.removeDisplay("foo");
  cs = null;
  finishTest();
}

