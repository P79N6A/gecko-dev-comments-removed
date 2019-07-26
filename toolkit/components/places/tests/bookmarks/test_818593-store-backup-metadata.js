







function run_test() {
  run_next_test();
}

add_task(function test_saveBookmarksToJSONFile_and_create()
{
  
  let uri = NetUtil.newURI("http://getfirefox.com/");
  let bookmarkId =
    PlacesUtils.bookmarks.insertBookmark(
      PlacesUtils.unfiledBookmarksFolderId, uri,
      PlacesUtils.bookmarks.DEFAULT_INDEX, "Get Firefox!");

  
  let backupFile = FileUtils.getFile("TmpD", ["bookmarks.json"]);
  backupFile.create(Ci.nsILocalFile.NORMAL_FILE_TYPE, parseInt("0600", 8));

  let nodeCount = yield PlacesBackups.saveBookmarksToJSONFile(backupFile, true);
  do_check_true(nodeCount > 0);
  do_check_true(backupFile.exists());
  do_check_eq(backupFile.leafName, "bookmarks.json");

  
  
  let recentBackup = yield PlacesBackups.getMostRecentBackup();
  let todayFilename = PlacesBackups.getFilenameForDate();
  do_check_eq(OS.Path.basename(recentBackup),
              todayFilename.replace(/\.json/, "_" + nodeCount + ".json"));

  
  yield PlacesBackups.create(0);
  do_check_eq((yield PlacesBackups.getBackupFiles()).length, 0);

  
  yield PlacesBackups.create();
  do_check_eq((yield PlacesBackups.getBackupFiles()).length, 1);

  mostRecentBackupFile = yield PlacesBackups.getMostRecentBackup();
  do_check_neq(mostRecentBackupFile, null);
  let rx = new RegExp("^" + todayFilename.replace(/\.json/, "") + "_([0-9]+)\.json$");
  let matches = OS.Path.basename(recentBackup).match(rx);
  do_check_true(matches.length > 0 && parseInt(matches[1]) == nodeCount);

  
  backupFile.remove(false);
  yield PlacesBackups.create(0);
  PlacesUtils.bookmarks.removeItem(bookmarkId);
});

