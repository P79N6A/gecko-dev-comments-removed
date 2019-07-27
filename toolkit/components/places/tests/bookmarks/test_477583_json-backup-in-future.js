





function run_test() {
  do_test_pending();

  Task.spawn(function() {
    let backupFolder = yield PlacesBackups.getBackupFolder();
    let bookmarksBackupDir = new FileUtils.File(backupFolder);
    
    let files = bookmarksBackupDir.directoryEntries;
    while (files.hasMoreElements()) {
      let entry = files.getNext().QueryInterface(Ci.nsIFile);
      entry.remove(false);
    }

    
    let dateObj = new Date();
    dateObj.setYear(dateObj.getFullYear() + 1);
    let name = PlacesBackups.getFilenameForDate(dateObj);
    do_check_eq(name, "bookmarks-" + PlacesBackups.toISODateString(dateObj) + ".json");
    files = bookmarksBackupDir.directoryEntries;
    while (files.hasMoreElements()) {
      let entry = files.getNext().QueryInterface(Ci.nsIFile);
      if (PlacesBackups.filenamesRegex.test(entry.leafName))
        entry.remove(false);
    }

    let futureBackupFile = bookmarksBackupDir.clone();
    futureBackupFile.append(name);
    futureBackupFile.create(Ci.nsILocalFile.NORMAL_FILE_TYPE, 0600);
    do_check_true(futureBackupFile.exists());

    do_check_eq((yield PlacesBackups.getBackupFiles()).length, 0);

    yield PlacesBackups.create();
    
    do_check_eq((yield PlacesBackups.getBackupFiles()).length, 1);
    let mostRecentBackupFile = yield PlacesBackups.getMostRecentBackup();
    do_check_neq(mostRecentBackupFile, null);
    let todayFilename = PlacesBackups.getFilenameForDate();
    do_check_true(PlacesBackups.filenamesRegex.test(OS.Path.basename(mostRecentBackupFile)));

    
    do_check_false(futureBackupFile.exists());

    
    mostRecentBackupFile = new FileUtils.File(mostRecentBackupFile);
    mostRecentBackupFile.remove(false);
    do_check_false(mostRecentBackupFile.exists());

    do_test_finished()
  });
}
