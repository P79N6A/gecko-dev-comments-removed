function run_test() {
  run_next_test();
}








add_task(function* test_same_date_same_hash() {
  
  
  let backupFolder = yield PlacesBackups.getBackupFolder();
  
  let tempPath = OS.Path.join(OS.Constants.Path.profileDir,
                              "bug10169583_bookmarks.json");
  let {count, hash} = yield BookmarkJSONUtils.exportToFile(tempPath);

  
  let dateObj = new Date();
  let filename = "bookmarks-" + PlacesBackups.toISODateString(dateObj) + "_" +
                  count + "_" + hash + ".json";
  let backupFile = OS.Path.join(backupFolder, filename);
  yield OS.File.move(tempPath, backupFile);

  
  yield PlacesBackups.create();
  let mostRecentBackupFile = yield PlacesBackups.getMostRecentBackup();
  
  Assert.equal(mostRecentBackupFile, backupFile);
  
  let converter = Cc["@mozilla.org/intl/scriptableunicodeconverter"].
                        createInstance(Ci.nsIScriptableUnicodeConverter);
  converter.charset = "UTF-8";
  let result = yield OS.File.read(mostRecentBackupFile);
  let jsonString = converter.convertFromByteArray(result, result.length);
  do_print("Check is valid JSON");
  JSON.parse(jsonString);

  
  yield OS.File.remove(backupFile);
  yield OS.File.remove(tempPath);
  PlacesBackups._backupFiles = null; 
});

add_task(function* test_same_date_diff_hash() {
  
  
  let backupFolder = yield PlacesBackups.getBackupFolder();
  let tempPath = OS.Path.join(OS.Constants.Path.profileDir,
                              "bug10169583_bookmarks.json");
  let {count, hash} = yield BookmarkJSONUtils.exportToFile(tempPath);
  let dateObj = new Date();
  let filename = "bookmarks-" + PlacesBackups.toISODateString(dateObj) + "_" +
                  count + "_" + "differentHash==" + ".json";
  let backupFile = OS.Path.join(backupFolder, filename);
  yield OS.File.move(tempPath, backupFile);
  yield PlacesBackups.create(); 
  mostRecentBackupFile = yield PlacesBackups.getMostRecentBackup();

  
  let converter = Cc["@mozilla.org/intl/scriptableunicodeconverter"].
                        createInstance(Ci.nsIScriptableUnicodeConverter);
  converter.charset = "UTF-8";
  let result = yield OS.File.read(mostRecentBackupFile, { compression: "lz4" });
  let jsonString = converter.convertFromByteArray(result, result.length);
  do_print("Check is valid JSON");
  JSON.parse(jsonString);

  
  yield OS.File.remove(mostRecentBackupFile);
  yield OS.File.remove(tempPath);
  PlacesBackups._backupFiles = null; 
});

add_task(function* test_diff_date_same_hash() {
  
  
  let backupFolder = yield PlacesBackups.getBackupFolder();
  let tempPath = OS.Path.join(OS.Constants.Path.profileDir,
                              "bug10169583_bookmarks.json");
  let {count, hash} = yield BookmarkJSONUtils.exportToFile(tempPath);
  let oldDate = new Date(2014, 1, 1);
  let curDate = new Date();
  let oldFilename = "bookmarks-" + PlacesBackups.toISODateString(oldDate) + "_" +
                  count + "_" + hash + ".json";
  let newFilename = "bookmarks-" + PlacesBackups.toISODateString(curDate) + "_" +
                  count + "_" + hash + ".json";
  let backupFile = OS.Path.join(backupFolder, oldFilename);
  let newBackupFile = OS.Path.join(backupFolder, newFilename);
  yield OS.File.move(tempPath, backupFile);

  
  yield PlacesBackups.create();
  let mostRecentBackupFile = yield PlacesBackups.getMostRecentBackup();
  Assert.equal(mostRecentBackupFile, newBackupFile);

  
  yield OS.File.remove(mostRecentBackupFile);
  yield OS.File.remove(tempPath);
});
