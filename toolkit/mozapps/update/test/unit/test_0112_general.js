







































const TEST_ID = "0112";



const MAX_TIME_DIFFERENCE = 60000;




const TEST_FILES = [
{
  description      : "Only added by update.manifest for complete updates " +
                     "when there is a channel change (add-cc)",
  fileName         : "channel-prefs.js",
  relPathDir       : "a/b/defaults/pref/",
  originalContents : "ShouldNotBeReplaced\n",
  compareContents  : "ShouldNotBeReplaced\n",
  originalFile     : null,
  compareFile      : null,
  originalPerms    : 0767,
  comparePerms     : null
}, {
  description      : "Not added for failed update (add)",
  fileName         : "precomplete",
  relPathDir       : "",
  originalContents : null,
  compareContents  : null,
  originalFile     : "data/complete_precomplete",
  compareFile      : "data/complete_precomplete",
  originalPerms    : 0666,
  comparePerms     : 0666
}, {
  description      : "Not added for failed update (add)",
  fileName         : "searchpluginstext0",
  relPathDir       : "a/b/searchplugins/",
  originalContents : "ShouldNotBeReplaced\n",
  compareContents  : "ShouldNotBeReplaced\n",
  originalFile     : null,
  compareFile      : null,
  originalPerms    : 0775,
  comparePerms     : 0775
}, {
  description      : "Not patched for failed update (patch-if)",
  fileName         : "searchpluginspng1.png",
  relPathDir       : "a/b/searchplugins/",
  originalContents : null,
  compareContents  : null,
  originalFile     : "data/complete.png",
  compareFile      : "data/complete.png",
  originalPerms    : 0666,
  comparePerms     : 0666
}, {
  description      : "Not patched for failed update (patch-if)",
  fileName         : "searchpluginspng0.png",
  relPathDir       : "a/b/searchplugins/",
  originalContents : null,
  compareContents  : null,
  originalFile     : "data/complete.png",
  compareFile      : "data/complete.png",
  originalPerms    : 0666,
  comparePerms     : null
}, {
  description      : "Not added for failed update (add-if)",
  fileName         : "extensions1text0",
  relPathDir       : "a/b/extensions/extensions1/",
  originalContents : null,
  compareContents  : null,
  originalFile     : null,
  compareFile      : null,
  originalPerms    : null,
  comparePerms     : null
}, {
  description      : "Not patched for failed update (patch-if)",
  fileName         : "extensions1png1.png",
  relPathDir       : "a/b/extensions/extensions1/",
  originalContents : null,
  compareContents  : null,
  originalFile     : "data/complete.png",
  compareFile      : "data/complete.png",
  originalPerms    : 0666,
  comparePerms     : 0666
}, {
  description      : "Not patched for failed update (patch-if)",
  fileName         : "extensions1png0.png",
  relPathDir       : "a/b/extensions/extensions1/",
  originalContents : null,
  compareContents  : null,
  originalFile     : "data/complete.png",
  compareFile      : "data/complete.png",
  originalPerms    : 0666,
  comparePerms     : 0666
}, {
  description      : "Not added for failed update (add-if)",
  fileName         : "extensions0text0",
  relPathDir       : "a/b/extensions/extensions0/",
  originalContents : "ShouldNotBeReplaced\n",
  compareContents  : "ShouldNotBeReplaced\n",
  originalFile     : null,
  compareFile      : null,
  originalPerms    : null,
  comparePerms     : 0644
}, {
  description      : "Not patched for failed update (patch-if)",
  fileName         : "extensions0png1.png",
  relPathDir       : "a/b/extensions/extensions0/",
  originalContents : null,
  compareContents  : null,
  originalFile     : "data/complete.png",
  compareFile      : "data/complete.png",
  originalPerms    : null,
  comparePerms     : 0644
}, {
  description      : "Not patched for failed update (patch-if)",
  fileName         : "extensions0png0.png",
  relPathDir       : "a/b/extensions/extensions0/",
  originalContents : null,
  compareContents  : null,
  originalFile     : "data/complete.png",
  compareFile      : "data/complete.png",
  originalPerms    : null,
  comparePerms     : 0644
}, {
  description      : "Not patched for failed update (patch)",
  fileName         : "exe0.exe",
  relPathDir       : "a/b/",
  originalContents : null,
  compareContents  : null,
  originalFile     : "data/complete.png",
  compareFile      : "data/complete.png",
  originalPerms    : 0755,
  comparePerms     : null
}, {
  description      : "Not patched for failed update (patch) and causes " +
                     "LoadSourceFile failed",
  fileName         : "0exe0.exe",
  relPathDir       : "a/b/0/",
  originalContents : null,
  compareContents  : null,
  originalFile     : "data/partial.png",
  compareFile      : "data/partial.png",
  originalPerms    : 0755,
  comparePerms     : null
}, {
  description      : "Not added for failed update (add)",
  fileName         : "00text0",
  relPathDir       : "a/b/0/00/",
  originalContents : "ShouldNotBeReplaced\n",
  compareContents  : "ShouldNotBeReplaced\n",
  originalFile     : null,
  compareFile      : null,
  originalPerms    : null,
  comparePerms     : null
}, {
  description      : "Not patched for failed update (patch)",
  fileName         : "00png0.png",
  relPathDir       : "a/b/0/00/",
  originalContents : null,
  compareContents  : null,
  originalFile     : "data/complete.png",
  compareFile      : "data/complete.png",
  originalPerms    : 0666,
  comparePerms     : 0666
}, {
  description      : "Not added for failed update (add)",
  fileName         : "20text0",
  relPathDir       : "a/b/2/20/",
  originalContents : null,
  compareContents  : null,
  originalFile     : null,
  compareFile      : null,
  originalPerms    : null,
  comparePerms     : null
}, {
  description      : "Not added for failed update (add)",
  fileName         : "20png0.png",
  relPathDir       : "a/b/2/20/",
  originalContents : null,
  compareContents  : null,
  originalFile     : null,
  compareFile      : null,
  originalPerms    : null,
  comparePerms     : null
}, {
  description      : "Not added for failed update (add)",
  fileName         : "00text2",
  relPathDir       : "a/b/0/00/",
  originalContents : null,
  compareContents  : null,
  originalFile     : null,
  compareFile      : null,
  originalPerms    : null,
  comparePerms     : null
}, {
  description      : "Not removed for failed update (remove)",
  fileName         : "10text0",
  relPathDir       : "a/b/1/10/",
  originalContents : "ShouldNotBeDeleted\n",
  compareContents  : "ShouldNotBeDeleted\n",
  originalFile     : null,
  compareFile      : null,
  originalPerms    : 0666,
  comparePerms     : 0666
}, {
  description      : "Not removed for failed update (remove)",
  fileName         : "00text1",
  relPathDir       : "a/b/0/00/",
  originalContents : "ShouldNotBeDeleted\n",
  compareContents  : "ShouldNotBeDeleted\n",
  originalFile     : null,
  compareFile      : null,
  originalPerms    : 0666,
  comparePerms     : 0666
}];

ADDITIONAL_TEST_DIRS = [
{
  description  : "Not removed for failed update (rmdir)",
  relPathDir   : "a/b/1/10/",
  dirRemoved   : false
}, {
  description  : "Not removed for failed update (rmdir)",
  relPathDir   : "a/b/1/",
  dirRemoved   : false
}];

function run_test() {
  if (IS_ANDROID) {
    logTestInfo("this test is not applicable to Android... returning early");
    return;
  }

  do_test_pending();
  do_register_cleanup(cleanupUpdaterTest);

  setupUpdaterTest(MAR_PARTIAL_FILE);

  let updatesDir = do_get_file(TEST_ID + UPDATES_DIR_SUFFIX);
  let applyToDir = getApplyDirFile();

  
  
  let force = updatesDir.clone();
  force.append(CHANNEL_CHANGE_FILE);
  force.create(AUS_Ci.nsIFile.FILE_TYPE, PERMS_FILE);

  
  
  
  if (IS_MACOSX) {
    let now = Date.now();
    let yesterday = now - (1000 * 60 * 60 * 24);
    applyToDir.lastModifiedTime = yesterday;
  }

  
  let exitValue = runUpdate();
  logTestInfo("testing updater binary process exitValue for success when " +
              "applying a partial mar");
  do_check_eq(exitValue, 0);

  logTestInfo("testing update.status should be " + STATE_FAILED);
  
  
  do_check_eq(readStatusFile(updatesDir).split(": ")[0], STATE_FAILED);

  
  
  if (IS_MACOSX) {
    logTestInfo("testing last modified time on the apply to directory has " +
                "changed after a successful update (bug 600098)");
    let now = Date.now();
    let timeDiff = Math.abs(applyToDir.lastModifiedTime - now);
    do_check_true(timeDiff < MAX_TIME_DIFFERENCE);
  }

  checkFilesAfterUpdateFailure();
  
  if (!IS_UNIX) {
    checkUpdateLogContents(LOG_PARTIAL_FAILURE);
  }

  logTestInfo("testing tobedeleted directory doesn't exist");
  let toBeDeletedDir = getApplyDirFile("tobedeleted", true);
  do_check_false(toBeDeletedDir.exists());

  checkCallbackAppLog();
}
