




let tmp = {};
Cu.import("resource://gre/modules/osfile.jsm", tmp);
Cu.import("resource://gre/modules/Task.jsm", tmp);
Cu.import("resource:///modules/sessionstore/_SessionFile.jsm", tmp);

const {OS, Task, _SessionFile} = tmp;

const PREF_SS_INTERVAL = "browser.sessionstore.interval";

const path = OS.Path.join(OS.Constants.Path.profileDir, "sessionstore.js");
const backupPath = OS.Path.join(OS.Constants.Path.profileDir,
  "sessionstore.bak");


let gDecoder = new TextDecoder();


let gSSData;
let gSSBakData;


function waitForSaveStateComplete(aSaveStateCallback) {
  let topic = "sessionstore-state-write-complete";

  function observer() {
    Services.prefs.clearUserPref(PREF_SS_INTERVAL);
    Services.obs.removeObserver(observer, topic);
    executeSoon(function taskCallback() {
      Task.spawn(aSaveStateCallback);
    });
  }

  Services.obs.addObserver(observer, topic, false);
}


function nextTest(testFunc) {
  waitForSaveStateComplete(testFunc);
  Services.prefs.setIntPref(PREF_SS_INTERVAL, 0);
}

registerCleanupFunction(function() {
  
  Task.spawn(function cleanupTask() {
    yield OS.File.remove(backupPath);
  });
});

function test() {
  waitForExplicitFinish();
  nextTest(testInitialWriteNoBackup);
}

function testInitialWriteNoBackup() {
  
  let ssExists = yield OS.File.exists(path);
  let ssBackupExists = yield OS.File.exists(backupPath);
  ok(ssExists, "sessionstore.js should be created.");
  ok(!ssBackupExists, "sessionstore.bak should not have been created, yet.");

  nextTest(testWriteNoBackup);
}

function testWriteNoBackup() {
  
  let ssExists = yield OS.File.exists(path);
  let ssBackupExists = yield OS.File.exists(backupPath);
  ok(ssExists, "sessionstore.js should exist.");
  ok(!ssBackupExists, "sessionstore.bak should not have been created, yet");

  
  
  let array = yield OS.File.read(path);
  gSSData = gDecoder.decode(array);

  
  
  yield _SessionFile.createBackupCopy();

  nextTest(testWriteBackup);
}

function testWriteBackup() {
  
  let ssExists = yield OS.File.exists(path);
  let ssBackupExists = yield OS.File.exists(backupPath);
  ok(ssExists, "sessionstore.js exists.");
  ok(ssBackupExists, "sessionstore.bak should now be created.");

  
  let array = yield OS.File.read(backupPath);
  gSSBakData = gDecoder.decode(array);

  
  
  is(gSSBakData, gSSData, "sessionstore.js is backed up correctly.");

  
  array = yield OS.File.read(path);
  gSSData = gDecoder.decode(array);

  
  let ssDataRead = yield _SessionFile.read();
  is(ssDataRead, gSSData, "_SessionFile.read read sessionstore.js correctly.");

  
  ssDataRead = _SessionFile.syncRead();
  is(ssDataRead, gSSData,
    "_SessionFile.syncRead read sessionstore.js correctly.");

  
  yield OS.File.remove(path);
  ssExists = yield OS.File.exists(path);
  ok(!ssExists, "sessionstore.js should be removed now.");

  
  ssDataRead = yield _SessionFile.read();
  is(ssDataRead, gSSBakData,
    "_SessionFile.read read sessionstore.bak correctly.");

  
  ssDataRead = _SessionFile.syncRead();
  is(ssDataRead, gSSBakData,
    "_SessionFile.syncRead read sessionstore.bak correctly.");
  nextTest(testNoWriteBackup);
}

function testNoWriteBackup() {
  

  
  let array = yield OS.File.read(backupPath);
  let ssBakData = gDecoder.decode(array);
  
  is(ssBakData, gSSBakData, "sessionstore.bak is unchanged.");

  executeSoon(finish);
}