









































const TEST_URI = "http://example.com/browser/toolkit/components/console/hudservice/tests/browser/test-console.html";

function test() {
  addTab(TEST_URI);
  browser.addEventListener("DOMContentLoaded", testStorageRecordManyEntries,
                           false);
}

function testStorageRecordManyEntries() {
  browser.removeEventListener("DOMContentLoaded",
                              testStorageRecordManyEntries, false);

  openConsole();

  let cs = HUDService.storage;

  cs.createDisplay("foo");

  var configArr = [];

  for (var i = 0; i < 1000; i++){
    let config = {
      logLevel: "network",
      message: "HumminaHummina!",
      activity: {
        stage: "barStage",
        data: "bar bar bar bar"
      }
    };
    configArr.push(config);
  }

  cs.recordEntries("foo", configArr);
  ok(cs.displayIndexes["foo"].length == 1000,
     "1000 entries in foo now");

  cs.removeDisplay("foo");

  finishTest();
}

