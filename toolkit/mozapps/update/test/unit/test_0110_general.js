







































function run_test() {
  
  
  var testDir = do_get_cwd();
  
  
  testDir.append("mar_test");
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
  testDir.create(AUS_Ci.nsIFile.DIRECTORY_TYPE, 0755);

  
  
  var testFile = testDir.clone();
  testFile.append("text1");
  testFile.create(AUS_Ci.nsIFile.NORMAL_FILE_TYPE, 0644);

  var binDir = gDirSvc.get(NS_GRE_DIR, AUS_Ci.nsIFile);

  
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

  
  var updatesSubDir = do_get_cwd();
  updatesSubDir.append("0110_complete_mar");

  try {
    
    
    if (updatesSubDir.exists())
      updatesSubDir.remove(true);
  }
  catch (e) {
    dump("Unable to remove directory\npath: " + updatesSubDir.path +
         "\nException: " + e + "\n");
  }

  var mar = do_get_file("data/aus-0110_general-1.mar");
  mar.copyTo(updatesSubDir, "update.mar");

  
  var exitValue = runUpdate(updatesSubDir, updater);
  dump("Testing: updater binary process exitValue for success when applying " +
       "a complete mar\n");
  do_check_eq(exitValue, 0);

  dump("Testing: contents of files added by a complete mar\n");
  do_check_eq(getFileBytes(getTestFile(testDir, "text1")), "ToBeModified\n");
  do_check_eq(getFileBytes(getTestFile(testDir, "text2")), "ToBeDeleted\n");

  var refImage = do_get_file("data/aus-0110_general_ref_image1.png");
  var srcImage = getTestFile(testDir, "image1.png");
  do_check_eq(getFileBytes(srcImage), getFileBytes(refImage));

  try {
    
    
    if (updatesSubDir.exists())
      updatesSubDir.remove(true);
  }
  catch (e) {
    dump("Unable to remove directory\npath: " + updatesSubDir.path +
         "\nException: " + e + "\n");
  }
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

  var process = AUS_Cc["@mozilla.org/process/util;1"]
                  .createInstance(AUS_Ci.nsIProcess);
  process.init(updateBin);
  var args = [updatesSubDirPath];
  process.run(true, args, args.length);
  return process.exitValue;
}



function getTestFile(aDir, aLeafName) {
  var file = aDir.clone();
  file.append(aLeafName);
  if (!(file instanceof AUS_Ci.nsILocalFile))
    do_throw("File must be a nsILocalFile for this test! File: " + aLeafName);

  return file;
}


function getFileBytes(aFile) {
  var fis = AUS_Cc["@mozilla.org/network/file-input-stream;1"]
              .createInstance(AUS_Ci.nsIFileInputStream);
  fis.init(aFile, -1, -1, false);
  var bis = AUS_Cc["@mozilla.org/binaryinputstream;1"]
              .createInstance(AUS_Ci.nsIBinaryInputStream);
  bis.setInputStream(fis);
  var data = bis.readBytes(bis.available());
  bis.close();
  fis.close();
  return data;
}
