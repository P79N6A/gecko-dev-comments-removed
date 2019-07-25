







































const APPLY_TO_DIR = "applyToDir_0110";
const UPDATES_DIR  = "0110_mar";
const AFTER_APPLY_DIR = "afterApplyDir";
const UPDATER_BIN_FILE = "updater" + BIN_SUFFIX;
const AFTER_APPLY_BIN_FILE = "TestAUSHelper" + BIN_SUFFIX;
const RELAUNCH_BIN_FILE = "relaunch_app" + BIN_SUFFIX;
const RELAUNCH_ARGS = ["Test Arg 1", "Test Arg 2", "Test Arg 3"];



const MAX_TIME_DIFFERENCE = 60000;

var gTestFiles = [
{
  fileName         : "1_exe1.exe",
  destinationDir   : APPLY_TO_DIR + "/mar_test/1/",
  originalContents : null,
  compareContents  : null,
  originalFile     : "data/aus-0111_general_ref_image.png",
  compareFile      : "data/aus-0110_general_ref_image.png",
  originalPerms    : 0777,
  comparePerms     : 0755
}, {
  fileName         : "1_1_image1.png",
  destinationDir   : APPLY_TO_DIR + "/mar_test/1/1_1/",
  originalContents : null,
  compareContents  : null,
  originalFile     : null,
  compareFile      : "data/aus-0110_general_ref_image.png",
  originalPerms    : 0776,
  comparePerms     : 0644
}, {
  fileName         : "1_1_text1",
  destinationDir   : APPLY_TO_DIR + "/mar_test/1/1_1/",
  originalContents : "ToBeReplacedWithToBeModified\n",
  compareContents  : "ToBeModified\n",
  originalFile     : null,
  compareFile      : null,
  originalPerms    : 0775,
  comparePerms     : 0644
}, {
  fileName         : "1_1_text2",
  destinationDir   : APPLY_TO_DIR + "/mar_test/1/1_1/",
  originalContents : "ToBeReplacedWithToBeDeleted\n",
  compareContents  : "ToBeDeleted\n",
  originalFile     : null,
  compareFile      : null,
  originalPerms    : 0677,
  comparePerms     : 0644
}, {
  fileName         : "2_1_text1",
  destinationDir   : APPLY_TO_DIR + "/mar_test/2/2_1/",
  originalContents : "ToBeReplacedWithToBeDeleted\n",
  compareContents  : "ToBeDeleted\n",
  originalFile     : null,
  compareFile      : null,
  originalPerms    : 0767,
  comparePerms     : 0644
}];

function run_test() {
  if (IS_ANDROID) {
    logTestInfo("this test is not applicable to Android... returning early");
    return;
  }

  do_test_pending();
  do_register_cleanup(end_test);

  var testDir, testFile;

  
  
  
  var applyToDir = do_get_file(APPLY_TO_DIR, true);

  
  try {
    removeDirRecursive(applyToDir);
  }
  catch (e) {
    dump("Unable to remove directory\n" +
         "path: " + applyToDir.path + "\n" +
         "Exception: " + e + "\n");
  }
  logTestInfo("testing successful removal of the directory used to apply the " +
              "mar file");
  do_check_false(applyToDir.exists());

  
  var updatesDir = do_get_file(UPDATES_DIR, true);
  try {
    
    
    removeDirRecursive(updatesDir);
  }
  catch (e) {
    dump("Unable to remove directory\n" +
         "path: " + updatesDir.path + "\n" +
         "Exception: " + e + "\n");
  }
  if (!updatesDir.exists()) {
    updatesDir.create(AUS_Ci.nsIFile.DIRECTORY_TYPE, PERMS_DIRECTORY);
  }

  
  for (var i = 0; i < gTestFiles.length; i++) {
    var f = gTestFiles[i];
    if (f.originalFile || f.originalContents) {
      testDir = do_get_file(f.destinationDir, true);
      if (!testDir.exists())
        testDir.create(AUS_Ci.nsIFile.DIRECTORY_TYPE, PERMS_DIRECTORY);

      if (f.originalFile) {
        testFile = do_get_file(f.originalFile);
        testFile.copyTo(testDir, f.fileName);
        testFile = do_get_file(f.destinationDir + f.fileName);
      }
      else {
        testFile = do_get_file(f.destinationDir + f.fileName, true);
        writeFile(testFile, f.originalContents);
      }

      
      
      if (!IS_WIN && !IS_OS2 && f.originalPerms) {
        testFile.permissions = f.originalPerms;
        
        
        if (!f.comparePerms)
          f.comparePerms = testFile.permissions;
      }
    }
  }

  var afterApplyBinDir = applyToDir.clone();
  afterApplyBinDir.append(AFTER_APPLY_DIR);

  var afterApplyBin = do_get_file(AFTER_APPLY_BIN_FILE);
  afterApplyBin.copyTo(afterApplyBinDir, RELAUNCH_BIN_FILE);

  var relaunchApp = afterApplyBinDir.clone();
  relaunchApp.append(RELAUNCH_BIN_FILE);
  relaunchApp.permissions = PERMS_DIRECTORY;

  let updaterIniContents = "[Strings]\n" +
                           "Title=Update Test\n" +
                           "Info=Application Update XPCShell Test - " +
                           "test_0110_general.js\n";
  var updaterIni = updatesDir.clone();
  updaterIni.append(FILE_UPDATER_INI);
  writeFile(updaterIni, updaterIniContents);
  updaterIni.copyTo(afterApplyBinDir, FILE_UPDATER_INI);

  
  
  
  if (IS_MACOSX) {
    var now = Date.now();
    var yesterday = now - (1000 * 60 * 60 * 24);
    applyToDir.lastModifiedTime = yesterday;
  }

  var binDir = getGREDir();

  
  var updater = binDir.clone();
  updater.append("updater.app");
  if (!updater.exists()) {
    updater = binDir.clone();
    updater.append(UPDATER_BIN_FILE);
    if (!updater.exists()) {
      do_throw("Unable to find updater binary!");
    }
  }

  var mar = do_get_file("data/aus-0110_general.mar");
  mar.copyTo(updatesDir, FILE_UPDATE_ARCHIVE);

  
  var exitValue = runUpdate(updater, updatesDir, applyToDir, relaunchApp,
                            RELAUNCH_ARGS);
  logTestInfo("testing updater binary process exitValue for success when " +
              "applying a complete mar");
  do_check_eq(exitValue, 0);

  logTestInfo("testing update.status should be " + STATE_SUCCEEDED);
  do_check_eq(readStatusFile(updatesDir), STATE_SUCCEEDED);

  
  
  if (IS_MACOSX) {
    logTestInfo("testing last modified time on the apply to directory has " +
                "changed after a successful update (bug 600098)");
    now = Date.now();
    var timeDiff = Math.abs(applyToDir.lastModifiedTime - now);
    do_check_true(timeDiff < MAX_TIME_DIFFERENCE);
  }

  logTestInfo("testing contents of files added by a complete mar");
  for (i = 0; i < gTestFiles.length; i++) {
    f = gTestFiles[i];
    testFile = do_get_file(f.destinationDir + f.fileName, true);
    logTestInfo("testing file: " + testFile.path);
    if (f.compareFile || f.compareContents) {
      do_check_true(testFile.exists());

      
      
      if (!IS_WIN && !IS_OS2 && f.comparePerms) {
        
        let logPerms = "testing file permissions - ";
        if (f.originalPerms) {
          logPerms += "original permissions: " + f.originalPerms.toString(8) + ", ";
        }
        logPerms += "compare permissions : " + f.comparePerms.toString(8) + ", ";
        logPerms += "updated permissions : " + testFile.permissions.toString(8);
        logTestInfo(logPerms);
        do_check_eq(testFile.permissions & 0xfff, f.comparePerms & 0xfff);
      }

      if (f.compareFile) {
        do_check_eq(readFileBytes(testFile),
                    readFileBytes(do_get_file(f.compareFile)));
        if (f.originalFile) {
          
          
          do_check_neq(readFileBytes(testFile),
                       readFileBytes(do_get_file(f.originalFile)));
        }
      }
      else {
        do_check_eq(readFileBytes(testFile), f.compareContents);
      }
    }
    else {
      do_check_false(testFile.exists());
    }
  }

  logTestInfo("testing patch files should not be left behind");
  var entries = updatesDir.QueryInterface(AUS_Ci.nsIFile).directoryEntries;
  while (entries.hasMoreElements()) {
    var entry = entries.getNext().QueryInterface(AUS_Ci.nsIFile);
    do_check_neq(getFileExtension(entry), "patch");
  }

  check_app_launch_log();
}

function end_test() {
  
  var applyToDir = do_get_file(APPLY_TO_DIR, true);
  try {
    removeDirRecursive(applyToDir);
  }
  catch (e) {
    dump("Unable to remove directory\n" +
         "path: " + applyToDir.path + "\n" +
         "Exception: " + e + "\n");
  }

  var updatesDir = do_get_file(UPDATES_DIR, true);
  try {
    removeDirRecursive(updatesDir);
  }
  catch (e) {
    dump("Unable to remove directory\n" +
         "path: " + updatesDir.path + "\n" +
         "Exception: " + e + "\n");
  }

  cleanUp();
}

function check_app_launch_log() {
  var appLaunchLog = do_get_file(APPLY_TO_DIR);
  appLaunchLog.append(AFTER_APPLY_DIR);
  appLaunchLog.append(RELAUNCH_BIN_FILE + ".log");
  if (!appLaunchLog.exists()) {
    do_timeout(0, check_app_launch_log);
    return;
  }

  var expectedLogContents = "executed\n" + RELAUNCH_ARGS.join("\n") + "\n";
  var logContents = readFile(appLaunchLog).replace(/\r\n/g, "\n");
  
  
  
  
  if (logContents != expectedLogContents) {
    do_timeout(0, check_app_launch_log);
    return;
  }

  logTestInfo("testing that the callback application successfully launched " +
              "and the expected command line arguments passed to it");
  do_check_eq(logContents, expectedLogContents);

  do_test_finished();
}
