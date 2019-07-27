





"use strict";

Cu.import("resource://gre/modules/TelemetryController.jsm", this);
Cu.import("resource://gre/modules/TelemetrySession.jsm", this);
Cu.import("resource://gre/modules/TelemetryArchive.jsm", this);
Cu.import("resource://gre/modules/TelemetrySend.jsm", this);
Cu.import("resource://gre/modules/XPCOMUtils.jsm", this);
Cu.import("resource://gre/modules/osfile.jsm", this);
Cu.import("resource://gre/modules/Task.jsm", this);
Cu.import("resource://gre/modules/Services.jsm", this);

XPCOMUtils.defineLazyGetter(this, "gPingsArchivePath", function() {
  return OS.Path.join(OS.Constants.Path.profileDir, "datareporting", "archived");
});

const PREF_TELEMETRY_ENABLED = "toolkit.telemetry.enabled";
const Telemetry = Cc["@mozilla.org/base/telemetry;1"].getService(Ci.nsITelemetry);





function fakeStorageQuota(aArchiveQuota) {
  let storage = Cu.import("resource://gre/modules/TelemetryStorage.jsm");
  storage.Policy.getArchiveQuota = () => aArchiveQuota;
}











let getArchivedPingsInfo = Task.async(function*() {
  let dirIterator = new OS.File.DirectoryIterator(gPingsArchivePath);
  let subdirs = (yield dirIterator.nextBatch()).filter(e => e.isDir);
  let archivedPings = [];

  
  for (let dir of subdirs) {
    let fileIterator = new OS.File.DirectoryIterator(dir.path);
    let files = (yield fileIterator.nextBatch()).filter(e => !e.isDir);

    
    for (let f of files) {
      let pingInfo = TelemetryStorage._testGetArchivedPingDataFromFileName(f.name);
      if (!pingInfo) {
        
        continue;
      }
      
      pingInfo.size = (yield OS.File.stat(f.path)).size;
      archivedPings.push(pingInfo);
    }
  }

  
  archivedPings.sort((a, b) => b.timestamp - a.timestamp);
  return archivedPings;
});

function run_test() {
  do_get_profile(true);
  Services.prefs.setBoolPref(PREF_TELEMETRY_ENABLED, true);
  run_next_test();
}

add_task(function* test_archivedPings() {
  
  

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
    data.id = yield TelemetryController.submitExternalPing(data.type, data.payload);
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

  
  TelemetryController.reset();

  let pingList = yield TelemetryArchive.promiseArchivedPingList();
  Assert.deepEqual(expectedPingList, pingList,
                   "Should have submitted pings in archive list after restart");
  yield checkLoadingPings();

  
  let writeToArchivedDir = Task.async(function*(dirname, filename, content, compressed) {
    const dirPath = OS.Path.join(gPingsArchivePath, dirname);
    yield OS.File.makeDir(dirPath, { ignoreExisting: true });
    const filePath = OS.Path.join(dirPath, filename);
    const options = { tmpPath: filePath + ".tmp", noOverwrite: false };
    if (compressed) {
      options.compression = "lz4";
    }
    yield OS.File.writeAtomic(filePath, content, options);
  });

  const FAKE_ID1 = "10000000-0123-0123-0123-0123456789a1";
  const FAKE_ID2 = "20000000-0123-0123-0123-0123456789a2";
  const FAKE_ID3 = "20000000-0123-0123-0123-0123456789a3";
  const FAKE_TYPE = "foo";

  
  yield writeToArchivedDir("xx", "foo.json", "{}");
  yield writeToArchivedDir("2010-02", "xx.xx.xx.json", "{}");
  
  yield writeToArchivedDir("2010-02", "1." + FAKE_ID1 + "." + FAKE_TYPE + ".json", "{}");
  
  yield writeToArchivedDir("2010-02", "2." + FAKE_ID1 + "." + FAKE_TYPE + ".json", "");
  
  yield writeToArchivedDir("2010-02", "3." + FAKE_ID2 + "." + FAKE_TYPE + ".json", "");
  
  yield writeToArchivedDir("2010-02", "4." + FAKE_ID3 + "." + FAKE_TYPE + ".jsonlz4", "");

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
  expectedPingList.push({
    id: FAKE_ID3,
    type: "foo",
    timestampCreated: 4,
  });
  expectedPingList.sort((a, b) => a.timestampCreated - b.timestampCreated);

  
  yield TelemetryController.reset();

  
  
  pingList = yield TelemetryArchive.promiseArchivedPingList();
  Assert.deepEqual(expectedPingList, pingList, "Should have picked up valid archived pings");
  yield checkLoadingPings();

  
  Assert.ok((yield promiseRejects(TelemetryArchive.promiseArchivedPingById(FAKE_ID1))),
            "Should have rejected invalid ping");
  Assert.ok((yield promiseRejects(TelemetryArchive.promiseArchivedPingById(FAKE_ID2))),
            "Should have rejected invalid ping");
});

add_task(function* test_archiveCleanup() {
  const PING_TYPE = "foo";

  
  yield OS.File.removeDir(gPingsArchivePath);

  Telemetry.getHistogramById("TELEMETRY_ARCHIVE_SCAN_PING_COUNT").clear();
  Telemetry.getHistogramById("TELEMETRY_ARCHIVE_DIRECTORIES_COUNT").clear();

  
  yield TelemetryController.reset();
  yield TelemetryStorage.testCleanupTaskPromise();
  yield TelemetryArchive.promiseArchivedPingList();

  let h = Telemetry.getHistogramById("TELEMETRY_ARCHIVE_SCAN_PING_COUNT").snapshot();
  Assert.equal(h.sum, 0, "Telemetry must report 0 pings scanned if no archive dir exists.");
  
  h = Telemetry.getHistogramById("TELEMETRY_ARCHIVE_EVICTED_OLD_DIRS").snapshot();
  Assert.equal(h.sum, 0, "Telemetry must report 0 evicted dirs if no archive dir exists.");

  let expectedPrunedInfo = [];
  let expectedNotPrunedInfo = [];

  let checkArchive = Task.async(function*() {
    
    for (let prunedInfo of expectedPrunedInfo) {
      yield Assert.rejects(TelemetryArchive.promiseArchivedPingById(prunedInfo.id),
                           "Ping " + prunedInfo.id + " should have been pruned.");
      const pingPath =
        TelemetryStorage._testGetArchivedPingPath(prunedInfo.id, prunedInfo.creationDate, PING_TYPE);
      Assert.ok(!(yield OS.File.exists(pingPath)), "The ping should not be on the disk anymore.");
    }

    
    for (let expectedInfo of expectedNotPrunedInfo) {
      Assert.ok((yield TelemetryArchive.promiseArchivedPingById(expectedInfo.id)),
                "Ping" + expectedInfo.id + " should be in the archive.");
    }
  });

  Telemetry.getHistogramById("TELEMETRY_ARCHIVE_SESSION_PING_COUNT").clear();

  
  let date = fakeNow(2010, 1, 1, 1, 0, 0);
  let pingId = yield TelemetryController.submitExternalPing(PING_TYPE, {}, {});
  expectedPrunedInfo.push({ id: pingId, creationDate: date });

  
  const oldestDirectoryDate = fakeNow(2010, 2, 1, 1, 0, 0);
  pingId = yield TelemetryController.submitExternalPing(PING_TYPE, {}, {});
  expectedNotPrunedInfo.push({ id: pingId, creationDate: oldestDirectoryDate });

  
  
  for (let month of [3, 4]) {
    for (let minute = 0; minute < 10; minute++) {
      date = fakeNow(2010, month, 1, 1, minute, 0);
      pingId = yield TelemetryController.submitExternalPing(PING_TYPE, {}, {});
      expectedNotPrunedInfo.push({ id: pingId, creationDate: date });
    }
  }

  
  h = Telemetry.getHistogramById("TELEMETRY_ARCHIVE_SESSION_PING_COUNT");
  Assert.equal(h.snapshot().sum, 22, "All the pings must be live-accumulated in the histogram.");
  
  Telemetry.getHistogramById("TELEMETRY_ARCHIVE_EVICTED_OLD_DIRS").clear();
  Telemetry.getHistogramById("TELEMETRY_ARCHIVE_OLDEST_DIRECTORY_AGE").clear();

  
  let now = fakeNow(2010, 7, 1, 1, 0, 0);
  
  yield TelemetryController.reset();
  
  yield TelemetryStorage.testCleanupTaskPromise();
  
  yield TelemetryArchive.promiseArchivedPingList();

  
  yield checkArchive();

  
  h = Telemetry.getHistogramById("TELEMETRY_ARCHIVE_SCAN_PING_COUNT").snapshot();
  Assert.equal(h.sum, 21, "The histogram must count all the pings in the archive.");
  
  h = Telemetry.getHistogramById("TELEMETRY_ARCHIVE_EVICTED_OLD_DIRS").snapshot();
  Assert.equal(h.sum, 1, "Telemetry must correctly report removed archive directories.");
  
  h = Telemetry.getHistogramById("TELEMETRY_ARCHIVE_DIRECTORIES_COUNT").snapshot();
  Assert.equal(h.sum, 3, "Telemetry must correctly report the remaining archive directories.");
  
  const oldestAgeInMonths = 5;
  h = Telemetry.getHistogramById("TELEMETRY_ARCHIVE_OLDEST_DIRECTORY_AGE").snapshot();
  Assert.equal(h.sum, oldestAgeInMonths,
               "Telemetry must correctly report age of the oldest directory in the archive.");

  
  
  Telemetry.getHistogramById("TELEMETRY_ARCHIVE_SIZE_MB").clear();
  Telemetry.getHistogramById("TELEMETRY_ARCHIVE_EVICTED_OVER_QUOTA").clear();
  Telemetry.getHistogramById("TELEMETRY_ARCHIVE_EVICTING_OVER_QUOTA_MS").clear();

  
  fakeNow(2010, 8, 1, 1, 0, 0);
  
  yield TelemetryController.reset();
  
  yield TelemetryStorage.testCleanupTaskPromise();
  
  yield TelemetryArchive.promiseArchivedPingList();

  
  expectedPrunedInfo.push(expectedNotPrunedInfo.shift());
  
  yield checkArchive();

  
  const archivedPingsInfo = yield getArchivedPingsInfo();
  let archiveSizeInBytes =
    archivedPingsInfo.reduce((lastResult, element) => lastResult + element.size, 0);

  
  h = Telemetry.getHistogramById("TELEMETRY_ARCHIVE_SIZE_MB").snapshot();
  Assert.equal(h.sum, Math.round(archiveSizeInBytes / 1024 / 1024),
               "Telemetry must report the correct archive size.");
  h = Telemetry.getHistogramById("TELEMETRY_ARCHIVE_EVICTED_OVER_QUOTA").snapshot();
  Assert.equal(h.sum, 0, "Telemetry must report 0 evictions if quota is not hit.");
  h = Telemetry.getHistogramById("TELEMETRY_ARCHIVE_EVICTING_OVER_QUOTA_MS").snapshot();
  Assert.equal(h.sum, 0, "Telemetry must report a null elapsed time if quota is not hit.");

  
  const testQuotaInBytes = archiveSizeInBytes * 0.8;
  fakeStorageQuota(testQuotaInBytes);

  
  
  const safeQuotaSize = testQuotaInBytes * 0.9;
  let sizeInBytes = 0;
  let pingsWithinQuota = [];
  let pingsOutsideQuota = [];

  for (let pingInfo of archivedPingsInfo) {
    sizeInBytes += pingInfo.size;
    if (sizeInBytes >= safeQuotaSize) {
      pingsOutsideQuota.push({ id: pingInfo.id, creationDate: new Date(pingInfo.timestamp) });
      continue;
    }
    pingsWithinQuota.push({ id: pingInfo.id, creationDate: new Date(pingInfo.timestamp) });
  }

  expectedNotPrunedInfo = pingsWithinQuota;
  expectedPrunedInfo = expectedPrunedInfo.concat(pingsOutsideQuota);

  
  yield TelemetryController.reset();
  yield TelemetryStorage.testCleanupTaskPromise();
  yield TelemetryArchive.promiseArchivedPingList();
  
  yield checkArchive();

  h = Telemetry.getHistogramById("TELEMETRY_ARCHIVE_EVICTED_OVER_QUOTA").snapshot();
  Assert.equal(h.sum, pingsOutsideQuota.length,
               "Telemetry must correctly report the over quota pings evicted from the archive.");
  h = Telemetry.getHistogramById("TELEMETRY_ARCHIVE_SIZE_MB").snapshot();
  Assert.equal(h.sum, 300, "Archive quota was hit, a special size must be reported.");

  
  yield TelemetryController.reset();
  yield TelemetryStorage.testCleanupTaskPromise();
  yield TelemetryArchive.promiseArchivedPingList();
  yield checkArchive();
});

add_task(function* test_clientId() {
  
  
  yield TelemetryController.setup();
  const clientId = TelemetryController.clientID;

  let id = yield TelemetryController.submitExternalPing("test-type", {}, {addClientId: true});
  let ping = yield TelemetryArchive.promiseArchivedPingById(id);

  Assert.ok(!!ping, "Should have loaded the ping.");
  Assert.ok("clientId" in ping, "Ping should have a client id.")
  Assert.ok(UUID_REGEX.test(ping.clientId), "Client id is in UUID format.");
  Assert.equal(ping.clientId, clientId, "Ping client id should match the global client id.");

  
  
  
  let promiseSetup = TelemetryController.reset();
  id = yield TelemetryController.submitExternalPing("test-type", {}, {addClientId: true});
  ping = yield TelemetryArchive.promiseArchivedPingById(id);
  Assert.equal(ping.clientId, clientId);

  
  yield promiseSetup;
});

add_task(function* test_InvalidPingType() {
  const TYPES = [
    "a",
    "-",
    "¿€€€?",
    "-foo-",
    "-moo",
    "zoo-",
    ".bar",
    "asfd.asdf",
  ];

  for (let type of TYPES) {
    let histogram = Telemetry.getKeyedHistogramById("TELEMETRY_INVALID_PING_TYPE_SUBMITTED");
    Assert.equal(histogram.snapshot(type).sum, 0,
                 "Should not have counted this invalid ping yet: " + type);
    Assert.ok(promiseRejects(TelemetryController.submitExternalPing(type, {})),
              "Ping type should have been rejected.");
    Assert.equal(histogram.snapshot(type).sum, 1,
                 "Should have counted this as an invalid ping type.");
  }
});

add_task(function* test_currentPingData() {
  yield TelemetrySession.setup();

  
  let h = Telemetry.getHistogramById("TELEMETRY_TEST_RELEASE_OPTOUT");
  h.clear();
  h.add(1);
  let k = Telemetry.getKeyedHistogramById("TELEMETRY_TEST_KEYED_RELEASE_OPTOUT");
  k.clear();
  k.add("a", 1);

  
  for (let subsession of [true, false]) {
    let ping = TelemetryController.getCurrentPingData(subsession);

    Assert.ok(!!ping, "Should have gotten a ping.");
    Assert.equal(ping.type, "main", "Ping should have correct type.");
    const expectedReason = subsession ? "gather-subsession-payload" : "gather-payload";
    Assert.equal(ping.payload.info.reason, expectedReason, "Ping should have the correct reason.");

    let id = "TELEMETRY_TEST_RELEASE_OPTOUT";
    Assert.ok(id in ping.payload.histograms, "Payload should have test count histogram.");
    Assert.equal(ping.payload.histograms[id].sum, 1, "Test count value should match.");
    id = "TELEMETRY_TEST_KEYED_RELEASE_OPTOUT";
    Assert.ok(id in ping.payload.keyedHistograms, "Payload should have keyed test histogram.");
    Assert.equal(ping.payload.keyedHistograms[id]["a"].sum, 1, "Keyed test value should match.");
  }
});

add_task(function* test_shutdown() {
  yield TelemetrySend.shutdown();
});
