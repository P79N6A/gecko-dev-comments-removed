






let OS = Cu.import("resource://gre/modules/osfile.jsm", {}).OS;
let {File, Constants, Path} = OS;

const PREF_SS_INTERVAL = "browser.sessionstore.interval";
const Paths = SessionFile.Paths;


let gDecoder = new TextDecoder();


let gSSData;
let gSSBakData;

function promiseRead(path) {
  return File.read(path, {encoding: "utf-8"});
}

add_task(function* init() {
  
  
  Services.prefs.setIntPref(PREF_SS_INTERVAL, 10000000);
  registerCleanupFunction(() => Services.prefs.clearUserPref(PREF_SS_INTERVAL));
});

add_task(function* test_creation() {
  let OLD_BACKUP = Path.join(Constants.Path.profileDir, "sessionstore.bak");
  let OLD_UPGRADE_BACKUP = Path.join(Constants.Path.profileDir, "sessionstore.bak-0000000");

  yield File.writeAtomic(OLD_BACKUP, "sessionstore.bak");
  yield File.writeAtomic(OLD_UPGRADE_BACKUP, "sessionstore upgrade backup");

  yield SessionFile.wipe();
  yield SessionFile.read(); 
  for (let k of Paths.loadOrder) {
    ok(!(yield File.exists(Paths[k])), "After wipe " + k + " sessionstore file doesn't exist");
  }
  ok(!(yield File.exists(OLD_BACKUP)), "After wipe, old backup doesn't exist");
  ok(!(yield File.exists(OLD_UPGRADE_BACKUP)), "After wipe, old upgrade backup doesn't exist");

  let URL_BASE = "http://example.com/?atomic_backup_test_creation=" + Math.random();
  let URL = URL_BASE + "?first_write";
  let tab = gBrowser.addTab(URL);

  info("Testing situation after a single write");
  yield promiseBrowserLoaded(tab.linkedBrowser);
  yield TabStateFlusher.flush(tab.linkedBrowser);
  yield SessionSaver.run();

  ok((yield File.exists(Paths.recovery)), "After write, recovery sessionstore file exists again");
  ok(!(yield File.exists(Paths.recoveryBackup)), "After write, recoveryBackup sessionstore doesn't exist");
  ok((yield promiseRead(Paths.recovery)).indexOf(URL) != -1, "Recovery sessionstore file contains the required tab");
  ok(!(yield File.exists(Paths.clean)), "After first write, clean shutdown sessionstore doesn't exist, since we haven't shutdown yet");

  info("Testing situation after a second write");
  let URL2 = URL_BASE + "?second_write";
  tab.linkedBrowser.loadURI(URL2);
  yield promiseBrowserLoaded(tab.linkedBrowser);
  yield TabStateFlusher.flush(tab.linkedBrowser);
  yield SessionSaver.run();

  ok((yield File.exists(Paths.recovery)), "After second write, recovery sessionstore file still exists");
  ok((yield promiseRead(Paths.recovery)).indexOf(URL2) != -1, "Recovery sessionstore file contains the latest url");
  ok((yield File.exists(Paths.recoveryBackup)), "After write, recoveryBackup sessionstore now exists");
  let backup = yield promiseRead(Paths.recoveryBackup);
  ok(backup.indexOf(URL2) == -1, "Recovery backup doesn't contain the latest url");
  ok(backup.indexOf(URL) != -1, "Recovery backup contains the original url");
  ok(!(yield File.exists(Paths.clean)), "After first write, clean shutdown sessinstore doesn't exist, since we haven't shutdown yet");

  info("Reinitialize, ensure that we haven't leaked sensitive files");
  yield SessionFile.read(); 
  yield SessionSaver.run();
  ok(!(yield File.exists(Paths.clean)), "After second write, clean shutdown sessonstore doesn't exist, since we haven't shutdown yet");
  ok(!(yield File.exists(Paths.upgradeBackup)), "After second write, clean shutdwn sessionstore doesn't exist, since we haven't shutdown yet");
  ok(!(yield File.exists(Paths.nextUpgradeBackup)), "After second write, clean sutdown sessionstore doesn't exist, since we haven't shutdown yet");

  gBrowser.removeTab(tab);
  yield SessionFile.wipe();
});

let promiseSource = Task.async(function*(name) {
  let URL = "http://example.com/?atomic_backup_test_recovery=" + Math.random() + "&name=" + name;
  let tab = gBrowser.addTab(URL);

  yield promiseBrowserLoaded(tab.linkedBrowser);
  yield TabStateFlusher.flush(tab.linkedBrowser);
  yield SessionSaver.run();
  gBrowser.removeTab(tab);

  let SOURCE = yield promiseRead(Paths.recovery);
  yield SessionFile.wipe();
  return SOURCE;
});

add_task(function* test_recovery() {
  yield SessionFile.wipe();
  info("Attempting to recover from the recovery file");
  let SOURCE = yield promiseSource("Paths.recovery");
  
  yield File.makeDir(Paths.backups);
  yield File.writeAtomic(Paths.recovery, SOURCE);
  is((yield SessionFile.read()).source, SOURCE, "Recovered the correct source from the recovery file");
  yield SessionFile.wipe();

  info("Corrupting recovery file, attempting to recover from recovery backup");
  SOURCE = yield promiseSource("Paths.recoveryBackup");
  yield File.makeDir(Paths.backups);
  yield File.writeAtomic(Paths.recoveryBackup, SOURCE);
  yield File.writeAtomic(Paths.recovery, "<Invalid JSON>");
  is((yield SessionFile.read()).source, SOURCE, "Recovered the correct source from the recovery file");
  yield SessionFile.wipe();
});

add_task(function* test_recovery_inaccessible() {
  
  if (AppConstants.platform != "macosx" && AppConstants.platform != "linux") {
    return;
  }

  info("Making recovery file inaccessible, attempting to recover from recovery backup");
  let SOURCE_RECOVERY = yield promiseSource("Paths.recovery");
  let SOURCE = yield promiseSource("Paths.recoveryBackup");
  yield File.makeDir(Paths.backups);
  yield File.writeAtomic(Paths.recoveryBackup, SOURCE);

  
  yield File.writeAtomic(Paths.recovery, SOURCE_RECOVERY);
  yield File.setPermissions(Paths.recovery, { unixMode: 0 });

  is((yield SessionFile.read()).source, SOURCE, "Recovered the correct source from the recovery file");
  yield File.setPermissions(Paths.recovery, { unixMode: 0644 });
});

add_task(function* test_clean() {
  yield SessionFile.wipe();
  let SOURCE = yield promiseSource("Paths.clean");
  yield File.writeAtomic(Paths.clean, SOURCE);
  yield SessionFile.read();
  yield SessionSaver.run();
  is((yield promiseRead(Paths.cleanBackup)), SOURCE, "After first read/write, clean shutdown file has been moved to cleanBackup");
});

add_task(function* cleanup() {
  yield SessionFile.wipe();
});

