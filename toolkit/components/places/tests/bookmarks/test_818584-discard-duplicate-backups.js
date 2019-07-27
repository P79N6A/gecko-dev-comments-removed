







function run_test() {
  run_next_test();
}

add_task(function() {
  
  let backupFolder = yield PlacesBackups.getBackupFolder();
  let dateObj = new Date();
  dateObj.setDate(dateObj.getDate() - 1);
  let oldBackupName = PlacesBackups.getFilenameForDate(dateObj);
  let oldBackup = OS.Path.join(backupFolder, oldBackupName);
  let {count: count, hash: hash} = yield BookmarkJSONUtils.exportToFile(oldBackup);
  do_check_true(count > 0);
  do_check_eq(hash.length, 24);
  oldBackupName = oldBackupName.replace(/\.json/, "_" + count + "_" + hash + ".json");
  yield OS.File.move(oldBackup, OS.Path.join(backupFolder, oldBackupName));

  
  
  
  yield PlacesBackups.create();

  
  let backupFiles = yield PlacesBackups.getBackupFiles();
  do_check_eq(backupFiles.length, 1);
  
  let matches = OS.Path.basename(backupFiles[0]).match(PlacesBackups.filenamesRegex);
  do_check_eq(matches[1], PlacesBackups.toISODateString(new Date()));
  do_check_eq(matches[2], count);
  do_check_eq(matches[3], hash);

  
  let bookmarkId = PlacesUtils.bookmarks.insertBookmark(PlacesUtils.bookmarks.bookmarksMenuFolder,
                                                        uri("http://foo.com"),
                                                        PlacesUtils.bookmarks.DEFAULT_INDEX,
                                                        "foo");
  
  
  yield PlacesBackups.create(undefined, true);
  do_check_eq(backupFiles.length, 1);
  recentBackup = yield PlacesBackups.getMostRecentBackup();
  do_check_neq(recentBackup, OS.Path.join(backupFolder, oldBackupName));
  matches = OS.Path.basename(recentBackup).match(PlacesBackups.filenamesRegex);
  do_check_eq(matches[1], PlacesBackups.toISODateString(new Date()));
  do_check_eq(matches[2], count + 1);
  do_check_neq(matches[3], hash);

  
  PlacesUtils.bookmarks.removeItem(bookmarkId);
  yield PlacesBackups.create(0);
});
