









const TEST_FILES = [
{
  description      : "Should never change",
  fileName         : "channel-prefs.js",
  relPathDir       : "a/b/defaults/pref/",
  originalContents : "ShouldNotBeReplaced\n",
  compareContents  : "ShouldNotBeReplaced\n",
  originalFile     : null,
  compareFile      : null,
  originalPerms    : 0o644,
  comparePerms     : null
}, {
  description      : "Added by update.manifest (add)",
  fileName         : "precomplete",
  relPathDir       : "",
  originalContents : null,
  compareContents  : null,
  originalFile     : "complete_precomplete",
  compareFile      : "partial_precomplete",
  originalPerms    : 0o666,
  comparePerms     : 0o644
}, {
  description      : "Added by update.manifest (add)",
  fileName         : "searchpluginstext0",
  relPathDir       : "a/b/searchplugins/",
  originalContents : "ToBeReplacedWithFromPartial\n",
  compareContents  : "FromPartial\n",
  originalFile     : null,
  compareFile      : null,
  originalPerms    : 0o775,
  comparePerms     : 0o644
}, {
  description      : "Patched by update.manifest if the file exists " +
                     "(patch-if)",
  fileName         : "searchpluginspng1.png",
  relPathDir       : "a/b/searchplugins/",
  originalContents : null,
  compareContents  : null,
  originalFile     : "complete.png",
  compareFile      : "partial.png",
  originalPerms    : 0o666,
  comparePerms     : 0o666
}, {
  description      : "Patched by update.manifest if the file exists " +
                     "(patch-if)",
  fileName         : "searchpluginspng0.png",
  relPathDir       : "a/b/searchplugins/",
  originalContents : null,
  compareContents  : null,
  originalFile     : "complete.png",
  compareFile      : "partial.png",
  originalPerms    : 0o666,
  comparePerms     : 0o666
}, {
  description      : "Added by update.manifest if the parent directory " +
                     "exists (add-if)",
  fileName         : "extensions1text0",
  relPathDir       : "a/b/extensions/extensions1/",
  originalContents : null,
  compareContents  : "FromPartial\n",
  originalFile     : null,
  compareFile      : null,
  originalPerms    : null,
  comparePerms     : 0o644
}, {
  description      : "Patched by update.manifest if the parent directory " +
                     "exists (patch-if)",
  fileName         : "extensions1png1.png",
  relPathDir       : "a/b/extensions/extensions1/",
  originalContents : null,
  compareContents  : null,
  originalFile     : "complete.png",
  compareFile      : "partial.png",
  originalPerms    : 0o666,
  comparePerms     : 0o666
}, {
  description      : "Patched by update.manifest if the parent directory " +
                     "exists (patch-if)",
  fileName         : "extensions1png0.png",
  relPathDir       : "a/b/extensions/extensions1/",
  originalContents : null,
  compareContents  : null,
  originalFile     : "complete.png",
  compareFile      : "partial.png",
  originalPerms    : 0o666,
  comparePerms     : 0o666
}, {
  description      : "Added by update.manifest if the parent directory " +
                     "exists (add-if)",
  fileName         : "extensions0text0",
  relPathDir       : "a/b/extensions/extensions0/",
  originalContents : "ToBeReplacedWithFromPartial\n",
  compareContents  : "FromPartial\n",
  originalFile     : null,
  compareFile      : null,
  originalPerms    : 0o644,
  comparePerms     : 0o644
}, {
  description      : "Patched by update.manifest if the parent directory " +
                     "exists (patch-if)",
  fileName         : "extensions0png1.png",
  relPathDir       : "a/b/extensions/extensions0/",
  originalContents : null,
  compareContents  : null,
  originalFile     : "complete.png",
  compareFile      : "partial.png",
  originalPerms    : 0o644,
  comparePerms     : 0o644
}, {
  description      : "Patched by update.manifest if the parent directory " +
                     "exists (patch-if)",
  fileName         : "extensions0png0.png",
  relPathDir       : "a/b/extensions/extensions0/",
  originalContents : null,
  compareContents  : null,
  originalFile     : "complete.png",
  compareFile      : "partial.png",
  originalPerms    : 0o644,
  comparePerms     : 0o644
}, {
  description      : "Patched by update.manifest (patch)",
  fileName         : "exe0.exe",
  relPathDir       : "a/b/",
  originalContents : null,
  compareContents  : null,
  originalFile     : "complete.png",
  compareFile      : "partial.png",
  originalPerms    : 0o755,
  comparePerms     : 0o755
}, {
  description      : "Patched by update.manifest (patch)",
  fileName         : "0exe0.exe",
  relPathDir       : "a/b/0/",
  originalContents : null,
  compareContents  : null,
  originalFile     : "complete.png",
  compareFile      : "partial.png",
  originalPerms    : 0o755,
  comparePerms     : 0o755
}, {
  description      : "Added by update.manifest (add)",
  fileName         : "00text0",
  relPathDir       : "a/b/0/00/",
  originalContents : "ToBeReplacedWithFromPartial\n",
  compareContents  : "FromPartial\n",
  originalFile     : null,
  compareFile      : null,
  originalPerms    : 0o644,
  comparePerms     : 0o644
}, {
  description      : "Patched by update.manifest (patch)",
  fileName         : "00png0.png",
  relPathDir       : "a/b/0/00/",
  originalContents : null,
  compareContents  : null,
  originalFile     : "complete.png",
  compareFile      : "partial.png",
  originalPerms    : 0o666,
  comparePerms     : 0o666
}, {
  description      : "Added by update.manifest (add)",
  fileName         : "20text0",
  relPathDir       : "a/b/2/20/",
  originalContents : null,
  compareContents  : "FromPartial\n",
  originalFile     : null,
  compareFile      : null,
  originalPerms    : null,
  comparePerms     : 0o644
}, {
  description      : "Added by update.manifest (add)",
  fileName         : "20png0.png",
  relPathDir       : "a/b/2/20/",
  originalContents : null,
  compareContents  : null,
  originalFile     : null,
  compareFile      : "partial.png",
  originalPerms    : null,
  comparePerms     : 0o644
}, {
  description      : "Added by update.manifest (add)",
  fileName         : "00text2",
  relPathDir       : "a/b/0/00/",
  originalContents : null,
  compareContents  : "FromPartial\n",
  originalFile     : null,
  compareFile      : null,
  originalPerms    : null,
  comparePerms     : 0o644
}, {
  description      : "Removed by update.manifest (remove)",
  fileName         : "10text0",
  relPathDir       : "a/b/1/10/",
  originalContents : "ToBeDeleted\n",
  compareContents  : null,
  originalFile     : null,
  compareFile      : null,
  originalPerms    : null,
  comparePerms     : null
}, {
  description      : "Removed by update.manifest (remove)",
  fileName         : "00text1",
  relPathDir       : "a/b/0/00/",
  originalContents : "ToBeDeleted\n",
  compareContents  : null,
  originalFile     : null,
  compareFile      : null,
  originalPerms    : null,
  comparePerms     : null
}];

ADDITIONAL_TEST_DIRS = [
{
  description  : "Removed by update.manifest (rmdir)",
  relPathDir   : "a/b/1/10/",
  dirRemoved   : true
}, {
  description  : "Removed by update.manifest (rmdir)",
  relPathDir   : "a/b/1/",
  dirRemoved   : true
}];

function run_test() {
  setupTestCommon(true);

  gBackgroundUpdate = true;
  setupUpdaterTest(FILE_PARTIAL_MAR);

  let updatesDir = do_get_file(gTestID + UPDATES_DIR_SUFFIX);
  let applyToDir = getApplyDirFile();

  
  
  
  if (IS_MACOSX) {
    let now = Date.now();
    let yesterday = now - (1000 * 60 * 60 * 24);
    applyToDir.lastModifiedTime = yesterday;
  }

  
  let exitValue = runUpdate();
  logTestInfo("testing updater binary process exitValue for success when " +
              "applying a partial mar");
  do_check_eq(exitValue, 0);

  logTestInfo("testing update.status should be " + STATE_APPLIED);
  do_check_eq(readStatusFile(updatesDir), STATE_APPLIED);

  
  
  if (IS_MACOSX) {
    logTestInfo("testing last modified time on the apply to directory has " +
                "changed after a successful update (bug 600098)");
    let now = Date.now();
    let timeDiff = Math.abs(applyToDir.lastModifiedTime - now);
    do_check_true(timeDiff < MAC_MAX_TIME_DIFFERENCE);
  }

  checkFilesAfterUpdateSuccess();
  
  if (!IS_UNIX) {
    checkUpdateLogContents(LOG_PARTIAL_SUCCESS);
  }

  
  logTestInfo("testing tobedeleted directory doesn't exist");
  let toBeDeletedDir = getApplyDirFile("tobedeleted", true);
  do_check_false(toBeDeletedDir.exists());
  toBeDeletedDir = getTargetDirFile("tobedeleted", true);
  do_check_false(toBeDeletedDir.exists());

  
  gBackgroundUpdate = false;
  gSwitchApp = true;
  exitValue = runUpdate();
  logTestInfo("testing updater binary process exitValue for success when " +
              "switching to the updated application");
  do_check_eq(exitValue, 0);

  logTestInfo("testing update.status should be " + STATE_SUCCEEDED);
  do_check_eq(readStatusFile(updatesDir), STATE_SUCCEEDED);

  
  
  if (IS_MACOSX) {
    logTestInfo("testing last modified time on the apply to directory has " +
                "changed after a successful update (bug 600098)");
    let now = Date.now();
    let timeDiff = Math.abs(applyToDir.lastModifiedTime - now);
    do_check_true(timeDiff < MAC_MAX_TIME_DIFFERENCE);
  }

  checkFilesAfterUpdateSuccess();
  
  if (!IS_UNIX) {
    checkUpdateLogContents(LOG_PARTIAL_SWITCH_SUCCESS);
  }

  
  logTestInfo("testing tobedeleted directory doesn't exist");
  toBeDeletedDir = getApplyDirFile("tobedeleted", true);
  do_check_false(toBeDeletedDir.exists());

  
  let updatedDir = applyToDir.clone();
  updatedDir.append(UPDATED_DIR_SUFFIX.replace("/", ""));
  do_check_false(updatedDir.exists());

  checkCallbackAppLog();
}

function end_test() {
  cleanupUpdaterTest();
}
