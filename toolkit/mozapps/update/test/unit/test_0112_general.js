







































function run_test() {
  
  
  var testDir = do_get_file("mar_test", true);
  
  
  try {
    if (testDir.exists())
      testDir.remove(true);
  }
  catch (e) {
    dump("Unable to remove directory\npath: " + testDir.path +
         "\nException: " + e + "\n");
  }
  dump("Testing: successful removal of the directory used to apply the mar file\n");
  do_check_false(testDir.exists());
  testDir = do_get_file("mar_test/1/1_1/", true);
  testDir.create(AUS_Ci.nsIFile.DIRECTORY_TYPE, PERMS_DIRECTORY);

  
  
  var testFile = do_get_file("mar_test/1/1_1/1_1_text1", true);
  writeFile(testFile, "ShouldNotBeModified\n");

  testFile = do_get_file("mar_test/1/1_1/1_1_text2", true);
  writeFile(testFile, "ShouldNotBeDeleted\n");

  testFile = do_get_file("data/aus-0111_general_ref_image.png");
  testFile.copyTo(testDir, "1_1_image1.png");

  testFile = do_get_file("mar_test/2/2_1/2_1_text1", true);
  writeFile(testFile, "ShouldNotBeDeleted\n");

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

  
  var updatesSubDir = do_get_file("0112_complete_mar", true);
  try {
    
    
    if (updatesSubDir.exists())
      updatesSubDir.remove(true);
  }
  catch (e) {
    dump("Unable to remove directory\npath: " + updatesSubDir.path +
         "\nException: " + e + "\n");
  }

  var mar = do_get_file("data/aus-0111_general.mar");
  mar.copyTo(updatesSubDir, "update.mar");

  
  var exitValue = runUpdate(updatesSubDir, updater);
  dump("Testing: updater binary process exitValue for success when applying " +
       "a partial mar\n");
  do_check_eq(exitValue, 0);

  dump("Testing: update.status should be set to STATE_FAILED\n");
  testFile = updatesSubDir.clone();
  testFile.append("update.status");
  
  
  do_check_eq(readFile(testFile).split(": ")[0], STATE_FAILED);

  dump("Testing: files should not be modified or deleted when an update " +
       "fails\n");
  do_check_eq(getFileBytes(do_get_file("mar_test/1/1_1/1_1_text1", true)),
              "ShouldNotBeModified\n");
  do_check_true(do_get_file("mar_test/1/1_1/1_1_text2", true).exists());
  do_check_eq(getFileBytes(do_get_file("mar_test/1/1_1/1_1_text2", true)),
              "ShouldNotBeDeleted\n");

  var refImage = do_get_file("data/aus-0111_general_ref_image.png");
  var srcImage = do_get_file("mar_test/1/1_1/1_1_image1.png", true);
  do_check_eq(getFileBytes(srcImage), getFileBytes(refImage));

  dump("Testing: removal of a file by a partial mar\n");
  do_check_true(do_get_file("mar_test/2/2_1/2_1_text1", true).exists());
  do_check_eq(getFileBytes(do_get_file("mar_test/1/1_1/1_1_text2", true)),
              "ShouldNotBeDeleted\n");

  do_check_false(do_get_file("mar_test/3/3_1/3_1_text1", true).exists());
  do_check_eq(getFileBytes(do_get_file("mar_test/1/1_1/1_1_text2", true)),
              "ShouldNotBeDeleted\n");

  try {
    
    
    if (updatesSubDir.exists())
      updatesSubDir.remove(true);
  }
  catch (e) {
    dump("Unable to remove directory\npath: " + updatesSubDir.path +
         "\nException: " + e + "\n");
  }

  cleanUp();
}


function runUpdate(aUpdatesSubDir, aUpdater) {
  
  
  
  aUpdater.copyTo(aUpdatesSubDir, aUpdater.leafName);
  var updateBin = aUpdatesSubDir.clone();
  updateBin.append(aUpdater.leafName);
  if (updateBin.leafName == "updater.app") {
    updateBin.append("Contents");
    updateBin.append("MacOS");
    updateBin.append("updater");
    if (!updateBin.exists())
      do_throw("Unable to find the updater executable!");
  }

  var updatesSubDirPath = aUpdatesSubDir.path;
  if (/ /.test(updatesSubDirPath))
    updatesSubDirPath = '"' + updatesSubDirPath + '"';

  var cwdPath = do_get_file("/", true).path;
  if (/ /.test(cwdPath))
    cwdPath = '"' + cwdPath + '"';

  var process = AUS_Cc["@mozilla.org/process/util;1"].
                createInstance(AUS_Ci.nsIProcess);
  process.init(updateBin);
  var args = [updatesSubDirPath, 0, cwdPath];
  process.run(true, args, args.length);
  return process.exitValue;
}


function getFileBytes(aFile) {
  var fis = AUS_Cc["@mozilla.org/network/file-input-stream;1"].
            createInstance(AUS_Ci.nsIFileInputStream);
  fis.init(aFile, -1, -1, false);
  var bis = AUS_Cc["@mozilla.org/binaryinputstream;1"].
            createInstance(AUS_Ci.nsIBinaryInputStream);
  bis.setInputStream(fis);
  var data = bis.readBytes(bis.available());
  bis.close();
  fis.close();
  return data;
}
