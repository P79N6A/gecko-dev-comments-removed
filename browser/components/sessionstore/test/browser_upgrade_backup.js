


Cu.import("resource://gre/modules/Services.jsm", this);
Cu.import("resource://gre/modules/osfile.jsm", this);
Cu.import("resource://gre/modules/Task.jsm", this);

function test() {
  waitForExplicitFinish();

  Task.spawn(function task() {
    try {
      
      yield SessionStore.promiseInitialized;

      const PREF_UPGRADE = "browser.sessionstore.upgradeBackup.latestBuildID";
      let buildID = Services.appinfo.platformBuildID;

      
      
      yield forceSaveState();

      
      Services.prefs.setCharPref(PREF_UPGRADE, "");
      let contents = "browser_upgrade_backup.js";
      let pathStore = OS.Path.join(OS.Constants.Path.profileDir, "sessionstore.js");
      yield OS.File.writeAtomic(pathStore, contents, { tmpPath: pathStore + ".tmp" });
      yield SessionStore._internal._performUpgradeBackup();
      is(Services.prefs.getCharPref(PREF_UPGRADE), buildID, "upgrade backup should be set (again)");

      let pathBackup = OS.Path.join(OS.Constants.Path.profileDir, "sessionstore.bak-" + Services.appinfo.platformBuildID);
      is((yield OS.File.exists(pathBackup)), true, "upgrade backup file has been created");

      let data = yield OS.File.read(pathBackup);
      is(new TextDecoder().decode(data), contents, "upgrade backup contains the expected contents");

      
      yield OS.File.writeAtomic(pathStore, "something else entirely", { tmpPath: pathStore + ".tmp" });
      yield SessionStore._internal._performUpgradeBackup();
      data = yield OS.File.read(pathBackup);
      is(new TextDecoder().decode(data), contents, "upgrade backup hasn't changed");

    } catch (ex) {
      ok(false, "Uncaught error: " + ex + " at " + ex.stack);
    } finally {
      finish();
    }
  });
}
