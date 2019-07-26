





function run_test() {
  run_next_test();
}

add_task(function* compress_bookmark_backups_test() {
  let backupFolder = yield PlacesBackups.getBackupFolder();

  
  let todayFilename = PlacesBackups.getFilenameForDate(new Date(2014, 04, 15), true);
  do_check_eq(todayFilename, "bookmarks-2014-05-15.jsonlz4");

  yield PlacesBackups.create();

  
  do_check_eq((yield PlacesBackups.getBackupFiles()).length, 1);
  let mostRecentBackupFile = yield PlacesBackups.getMostRecentBackup();
  do_check_neq(mostRecentBackupFile, null);
  do_check_true(PlacesBackups.filenamesRegex.test(OS.Path.basename(mostRecentBackupFile)));

  
  
  
  yield OS.File.remove(mostRecentBackupFile);
  do_check_false((yield OS.File.exists(mostRecentBackupFile)));

  
  
  let jsonFile = OS.Path.join(OS.Constants.Path.profileDir, "bookmarks.json");
  yield PlacesBackups.saveBookmarksToJSONFile(jsonFile);
  do_check_eq((yield PlacesBackups.getBackupFiles()).length, 1);

  
  let uri = NetUtil.newURI("http://www.mozilla.org/en-US/");
  let bm  = PlacesUtils.bookmarks.insertBookmark(PlacesUtils.unfiledBookmarksFolderId,
                                                 uri,
                                                 PlacesUtils.bookmarks.DEFAULT_INDEX,
                                                 "bookmark");

  
  yield PlacesBackups.create(undefined, true);
  let recentBackup = yield PlacesBackups.getMostRecentBackup();
  PlacesUtils.bookmarks.removeItem(bm);
  yield BookmarkJSONUtils.importFromFile(recentBackup, true);
  let root = PlacesUtils.getFolderContents(PlacesUtils.unfiledBookmarksFolderId).root;
  let node = root.getChild(0);
  do_check_eq(node.uri, uri.spec);

  root.containerOpen = false;
  PlacesUtils.bookmarks.removeFolderChildren(PlacesUtils.unfiledBookmarksFolderId);

  
  yield OS.File.remove(jsonFile);
});
