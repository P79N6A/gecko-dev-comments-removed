


Cu.import("resource://gre/modules/Services.jsm", this);
Cu.import("resource://gre/modules/osfile.jsm", this);
Cu.import("resource://gre/modules/Task.jsm", this);

const Paths = SessionFile.Paths;

add_task(function* init() {
  
  yield SessionStore.promiseInitialized;
  yield SessionFile.wipe();
});

add_task(function* test_upgrade_backup() {
  const PREF_UPGRADE = "browser.sessionstore.upgradeBackup.latestBuildID";
  let buildID = Services.appinfo.platformBuildID;
  info("Let's check if we create an upgrade backup");
  Services.prefs.setCharPref(PREF_UPGRADE, "");
  let contents = JSON.stringify({"browser_upgrade_backup.js": Math.random()});
  yield OS.File.writeAtomic(Paths.clean, contents);
  yield SessionFile.read(); 
  yield SessionFile.write(""); 

  is(Services.prefs.getCharPref(PREF_UPGRADE), buildID, "upgrade backup should be set");

  is((yield OS.File.exists(Paths.upgradeBackup)), true, "upgrade backup file has been created");

  let data = yield OS.File.read(Paths.upgradeBackup);
  is(contents, (new TextDecoder()).decode(data), "upgrade backup contains the expected contents");

  info("Let's check that we don't overwrite this upgrade backup");
  let new_contents = JSON.stringify({"something else entirely": Math.random()});
  yield OS.File.writeAtomic(Paths.clean, new_contents);
  yield SessionFile.read(); 
  yield SessionFile.write(""); 
  data = yield OS.File.read(Paths.upgradeBackup);
  is(contents, (new TextDecoder()).decode(data), "upgrade backup hasn't changed");
});
