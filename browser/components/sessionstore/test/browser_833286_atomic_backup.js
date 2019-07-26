






let tmp = {};
Cu.import("resource://gre/modules/osfile.jsm", tmp);
Cu.import("resource:///modules/sessionstore/SessionFile.jsm", tmp);

const {OS, SessionFile} = tmp;

const PREF_SS_INTERVAL = "browser.sessionstore.interval";

const path = OS.Path.join(OS.Constants.Path.profileDir, "sessionstore.js");
const backupPath = OS.Path.join(OS.Constants.Path.profileDir,
  "sessionstore.bak");


let gDecoder = new TextDecoder();


let gSSData;
let gSSBakData;



add_task(function* testAfterFirstWrite() {
  
  
  
  let ssExists = yield OS.File.exists(path);
  let ssBackupExists = yield OS.File.exists(backupPath);
  ok(ssExists, "sessionstore.js should exist.");
  ok(!ssBackupExists, "sessionstore.bak should not have been created, yet");

  
  
  let array = yield OS.File.read(path);
  gSSData = gDecoder.decode(array);

  
  
  yield OS.File.move(path, backupPath);

  yield forceSaveState();
});

add_task(function* testReadBackup() {
  
  let ssExists = yield OS.File.exists(path);
  let ssBackupExists = yield OS.File.exists(backupPath);
  ok(ssExists, "sessionstore.js exists.");
  ok(ssBackupExists, "sessionstore.bak should now be created.");

  
  let array = yield OS.File.read(backupPath);
  gSSBakData = gDecoder.decode(array);

  
  
  is(gSSBakData, gSSData, "sessionstore.js is backed up correctly.");

  
  array = yield OS.File.read(path);
  gSSData = gDecoder.decode(array);

  
  let ssDataRead = yield SessionFile.read();
  is(ssDataRead, gSSData, "SessionFile.read read sessionstore.js correctly.");

  
  yield OS.File.remove(path);
  ssExists = yield OS.File.exists(path);
  ok(!ssExists, "sessionstore.js should be removed now.");

  
  ssDataRead = yield SessionFile.read();
  is(ssDataRead, gSSBakData,
    "SessionFile.read read sessionstore.bak correctly.");

  yield forceSaveState();
});

add_task(function* testBackupUnchanged() {
  

  
  let array = yield OS.File.read(backupPath);
  let ssBakData = gDecoder.decode(array);
  
  is(ssBakData, gSSBakData, "sessionstore.bak is unchanged.");
});

add_task(function* cleanup() {
  
  yield OS.File.remove(backupPath);
});
