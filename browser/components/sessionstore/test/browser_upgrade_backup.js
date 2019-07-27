


Cu.import("resource://gre/modules/Services.jsm", this);
Cu.import("resource://gre/modules/osfile.jsm", this);
Cu.import("resource://gre/modules/Task.jsm", this);
Cu.import("resource://gre/modules/Preferences.jsm", this);

const Paths = SessionFile.Paths;
const PREF_UPGRADE = "browser.sessionstore.upgradeBackup.latestBuildID";
const PREF_MAX_UPGRADE_BACKUPS = "browser.sessionstore.upgradeBackup.maxUpgradeBackups";






let prepareTest = Task.async(function* () {
  let result = {};

  result.buildID = Services.appinfo.platformBuildID;
  Services.prefs.setCharPref(PREF_UPGRADE, "");
  result.contents = JSON.stringify({"browser_upgrade_backup.js": Math.random()});

  return result;
});




let getUpgradeBackups = Task.async(function* () {
  let iterator;
  let backups = [];
  let upgradeBackupPrefix = Paths.upgradeBackupPrefix;

  try {
    iterator = new OS.File.DirectoryIterator(Paths.backups);

    
    yield iterator.forEach(function (file) {
      
      if (file.path.startsWith(Paths.upgradeBackupPrefix)) {
        
        backups.push(file.path);
      }
    }, this);
  } finally {
    if (iterator) {
      iterator.close();
    }
  }

  
  return backups;
});

add_task(function* init() {
  
  yield SessionStore.promiseInitialized;
  yield SessionFile.wipe();
});

add_task(function* test_upgrade_backup() {
  let test = yield prepareTest();
  info("Let's check if we create an upgrade backup");
  yield OS.File.writeAtomic(Paths.clean, test.contents);
  yield SessionFile.read(); 
  yield SessionFile.write(""); 

  is(Services.prefs.getCharPref(PREF_UPGRADE), test.buildID, "upgrade backup should be set");

  is((yield OS.File.exists(Paths.upgradeBackup)), true, "upgrade backup file has been created");

  let data = yield OS.File.read(Paths.upgradeBackup);
  is(test.contents, (new TextDecoder()).decode(data), "upgrade backup contains the expected contents");

  info("Let's check that we don't overwrite this upgrade backup");
  let newContents = JSON.stringify({"something else entirely": Math.random()});
  yield OS.File.writeAtomic(Paths.clean, newContents);
  yield SessionFile.read(); 
  yield SessionFile.write(""); 
  data = yield OS.File.read(Paths.upgradeBackup);
  is(test.contents, (new TextDecoder()).decode(data), "upgrade backup hasn't changed");
});

add_task(function* test_upgrade_backup_removal() {
  let test = yield prepareTest();
  let maxUpgradeBackups = Preferences.get(PREF_MAX_UPGRADE_BACKUPS, 3);
  info("Let's see if we remove backups if there are too many");
  yield OS.File.writeAtomic(Paths.clean, test.contents);

  
  if (OS.File.exists(Paths.nextUpgradeBackup)) {
    yield OS.File.remove(Paths.nextUpgradeBackup);
  }

  
  yield OS.File.writeAtomic(Paths.upgradeBackupPrefix + "20080101010101", "");
  yield OS.File.writeAtomic(Paths.upgradeBackupPrefix + "20090101010101", "");
  yield OS.File.writeAtomic(Paths.upgradeBackupPrefix + "20100101010101", "");
  yield OS.File.writeAtomic(Paths.upgradeBackupPrefix + "20110101010101", "");
  yield OS.File.writeAtomic(Paths.upgradeBackupPrefix + "20120101010101", "");
  yield OS.File.writeAtomic(Paths.upgradeBackupPrefix + "20130101010101", "");

  
  let backups = yield getUpgradeBackups();

  
  yield SessionFile.read(); 
  yield SessionFile.write(""); 

  
  is(Services.prefs.getCharPref(PREF_UPGRADE), test.buildID, "upgrade backup should be set");
  is((yield OS.File.exists(Paths.upgradeBackup)), true, "upgrade backup file has been created");

  
  let newBackups = yield getUpgradeBackups();
  is(newBackups.length, maxUpgradeBackups, "expected number of backups are present after removing old backups");

  
  
  newBackups = newBackups.filter(function (backup) {
    return backups.indexOf(backup) < 0;
  });

  
  is(newBackups.length, 1, "one new backup was created that was not removed");

  yield SessionFile.write(""); 

  backups = yield getUpgradeBackups();
  is(backups.length, maxUpgradeBackups, "second call to SessionFile.write() didn't create or remove more backups");
});

