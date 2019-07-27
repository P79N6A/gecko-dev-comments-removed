





"use strict";

Cu.import("resource://gre/modules/TelemetryController.jsm", this);
Cu.import("resource://gre/modules/TelemetryArchive.jsm", this);
Cu.import("resource://gre/modules/XPCOMUtils.jsm", this);
Cu.import("resource://gre/modules/osfile.jsm", this);
Cu.import("resource://gre/modules/Task.jsm", this);
Cu.import("resource://gre/modules/Services.jsm", this);

XPCOMUtils.defineLazyGetter(this, "gPingsArchivePath", function() {
  return OS.Path.join(OS.Constants.Path.profileDir, "datareporting", "archived");
});


function run_test() {
  do_get_profile(true);
  run_next_test();
}

add_task(function* test_archivedPings() {
  yield TelemetryController.setup();

  const PINGS = [
    {
      type: "test-ping-api-1",
      payload: { foo: "bar"},
      dateCreated: new Date(2010, 1, 1, 10, 0, 0),
    },
    {
      type: "test-ping-api-2",
      payload: { moo: "meh"},
      dateCreated: new Date(2010, 2, 1, 10, 0, 0),
    },
  ];

  
  let expectedPingList = [];

  for (let data of PINGS) {
    fakeNow(data.dateCreated);
    data.id = yield TelemetryController.send(data.type, data.payload);
    let list = yield TelemetryArchive.promiseArchivedPingList();

    expectedPingList.push({
      id: data.id,
      type: data.type,
      timestampCreated: data.dateCreated.getTime(),
    });
    Assert.deepEqual(list, expectedPingList, "Archived ping list should contain submitted pings");
  }

  
  let ids = [for (p of PINGS) p.id];
  let checkLoadingPings = Task.async(function*() {
    for (let data of PINGS) {
      let ping = yield TelemetryArchive.promiseArchivedPingById(data.id);
      Assert.equal(ping.id, data.id, "Archived ping should have matching id");
      Assert.equal(ping.type, data.type, "Archived ping should have matching type");
      Assert.equal(ping.creationDate, data.dateCreated.toISOString(),
                   "Archived ping should have matching creation date");
    }
  });

  yield checkLoadingPings();

  
  yield TelemetryController.setup();

  let pingList = yield TelemetryArchive.promiseArchivedPingList();
  Assert.deepEqual(expectedPingList, pingList,
                   "Should have submitted pings in archive list after restart");
  yield checkLoadingPings();

  
  let writeToArchivedDir = Task.async(function*(dirname, filename, content) {
    const dirPath = OS.Path.join(gPingsArchivePath, dirname);
    yield OS.File.makeDir(dirPath, { ignoreExisting: true });
    const filePath = OS.Path.join(dirPath, filename);
    const options = { tmpPath: filePath + ".tmp", noOverwrite: false };
    yield OS.File.writeAtomic(filePath, content, options);
  });

  const FAKE_ID1 = "10000000-0123-0123-0123-0123456789a1";
  const FAKE_ID2 = "20000000-0123-0123-0123-0123456789a2";
  const FAKE_TYPE = "foo"

  
  yield writeToArchivedDir("xx", "foo.json", "{}");
  yield writeToArchivedDir("2010-02", "xx.xx.xx.json", "{}");
  
  yield writeToArchivedDir("2010-02", "1." + FAKE_ID1 + "." + FAKE_TYPE + ".json", "{}");
  
  yield writeToArchivedDir("2010-02", "2." + FAKE_ID1 + "." + FAKE_TYPE + ".json", "");
  
  yield writeToArchivedDir("2010-02", "3." + FAKE_ID2 + "." + FAKE_TYPE + ".json", "");

  expectedPingList.push({
    id: FAKE_ID1,
    type: "foo",
    timestampCreated: 2,
  });
  expectedPingList.push({
    id: FAKE_ID2,
    type: "foo",
    timestampCreated: 3,
  });
  expectedPingList.sort((a, b) => a.timestampCreated - b.timestampCreated);

  
  yield TelemetryArchive._testReset();

  
  
  pingList = yield TelemetryArchive.promiseArchivedPingList();
  Assert.deepEqual(expectedPingList, pingList, "Should have picked up valid archived pings");
  yield checkLoadingPings();

  
  Assert.ok((yield promiseRejects(TelemetryArchive.promiseArchivedPingById(FAKE_ID1))),
            "Should have rejected invalid ping");
  Assert.ok((yield promiseRejects(TelemetryArchive.promiseArchivedPingById(FAKE_ID2))),
            "Should have rejected invalid ping");
});
