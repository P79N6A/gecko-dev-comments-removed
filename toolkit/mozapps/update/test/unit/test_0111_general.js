







































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
  writeFile(testFile, "ToBeModified\n");

  testFile = testDir.clone();
  testFile.append("text2");
  writeFile(testFile, "ToBeDeleted\n");

  testFile = do_get_file("data/aus-0110_general_ref_image1.png");
  testFile.copyTo(testDir, "image1.png");

  var binDir = gRealGreD.clone();

  
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
  updatesSubDir.append("0111_partial_mar");

  try {
    
    
    if (updatesSubDir.exists())
      updatesSubDir.remove(true);
  }
  catch (e) {
    dump("Unable to remove directory\npath: " + updatesSubDir.path +
         "\nException: " + e + "\n");
  }

  var mar = do_get_file("data/aus-0110_general-2.mar");
  mar.copyTo(updatesSubDir, "update.mar");

  
  exitValue = runUpdate(updatesSubDir, updater);
  dump("Testing: updater binary process exitValue for success when applying " +
       "a partial mar\n");
  do_check_eq(exitValue, 0);

  dump("Testing: removal of a file and contents of added / modified files by " +
       "a partial mar\n");
  do_check_eq(getFileBytes(getTestFile(testDir, "text1")), "Modified\n");
  do_check_false(getTestFile(testDir, "text2").exists()); 
  do_check_eq(getFileBytes(getTestFile(testDir, "text3")), "Added\n");

  refImage = do_get_file("data/aus-0110_general_ref_image2.png");
  srcImage = getTestFile(testDir, "image1.png");
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
