







































var gTestFiles = [
{
  fileName         : "1_exe1.exe",
  destinationDir   : "mar_test/1/",
  originalContents : null,
  compareContents  : null,
  originalFile     : "data/aus-0111_general_ref_image.png",
  compareFile      : "data/aus-0110_general_ref_image.png",
  originalPerms    : 0777,
  comparePerms     : 0755
}, {
  fileName         : "1_1_image1.png",
  destinationDir   : "mar_test/1/1_1/",
  originalContents : null,
  compareContents  : null,
  originalFile     : null,
  compareFile      : "data/aus-0110_general_ref_image.png",
  originalPerms    : 0776,
  comparePerms     : 0644
}, {
  fileName         : "1_1_text1",
  destinationDir   : "mar_test/1/1_1/",
  originalContents : "ToBeReplacedWithToBeModified\n",
  compareContents  : "ToBeModified\n",
  originalFile     : null,
  compareFile      : null,
  originalPerms    : 0775,
  comparePerms     : 0644
}, {
  fileName         : "1_1_text2",
  destinationDir   : "mar_test/1/1_1/",
  originalContents : "ToBeReplacedWithToBeDeleted\n",
  compareContents  : "ToBeDeleted\n",
  originalFile     : null,
  compareFile      : null,
  originalPerms    : 0677,
  comparePerms     : 0644
}, {
  fileName         : "2_1_text1",
  destinationDir   : "mar_test/2/2_1/",
  originalContents : "ToBeReplacedWithToBeDeleted\n",
  compareContents  : "ToBeDeleted\n",
  originalFile     : null,
  compareFile      : null,
  originalPerms    : 0767,
  comparePerms     : 0644
}];

function run_test() {
  var testFile;
  
  
  var testDir = do_get_file("mar_test", true);
  
  
  try {
    removeDirRecursive(testDir);
  }
  catch (e) {
    dump("Unable to remove directory\npath: " + testDir.path +
         "\nException: " + e + "\n");
  }
  dump("Testing: successful removal of the directory used to apply the mar file\n");
  do_check_false(testDir.exists());

  
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

  var binDir = getGREDir();

  
  var updater = binDir.clone();
  updater.append("updater.app");
  if (!updater.exists()) {
    updater = binDir.clone();
    updater.append("updater.exe");
    if (!updater.exists()) {
      updater = binDir.clone();
      updater.append("updater");
      if (!updater.exists()) {
        do_throw("Unable to find updater binary!");
      }
    }
  }

  
  var updatesDir = do_get_file("0110_complete_mar", true);
  try {
    
    
    removeDirRecursive(updatesDir);
  }
  catch (e) {
    dump("Unable to remove directory\npath: " + updatesDir.path +
         "\nException: " + e + "\n");
  }

  var mar = do_get_file("data/aus-0110_general.mar");
  mar.copyTo(updatesDir, "update.mar");

  
  var exitValue = runUpdate(updatesDir, updater);
  dump("Testing: updater binary process exitValue for success when applying " +
       "a complete mar\n");
  do_check_eq(exitValue, 0);

  dump("Testing: update.status should be set to STATE_SUCCEEDED\n");
  testFile = updatesDir.clone();
  testFile.append("update.status");
  do_check_eq(readFile(testFile).split("\n")[0], STATE_SUCCEEDED);

  dump("Testing: contents of files added by a complete mar\n");
  for (i = 0; i < gTestFiles.length; i++) {
    f = gTestFiles[i];
    testFile = do_get_file(f.destinationDir + f.fileName, true);
    dump("Testing: " + testFile.path + "\n");
    if (f.compareFile || f.compareContents) {
      do_check_true(testFile.exists());

      
      
      if (!IS_WIN && !IS_OS2 && f.comparePerms) {
        
        if (f.originalPerms)
          dump("original permissions: " + f.originalPerms.toString(8) + "\n");
        dump("compare permissions : " + f.comparePerms.toString(8) + "\n");
        dump("updated permissions : " + testFile.permissions.toString(8) + "\n");
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

  cleanUp();
}
