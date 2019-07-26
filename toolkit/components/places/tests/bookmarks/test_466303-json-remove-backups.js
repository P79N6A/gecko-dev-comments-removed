





const NUMBER_OF_BACKUPS = 1;

function run_test() {
  do_test_pending();

  Task.spawn(function() {
    
    let backupFolder = yield PlacesBackups.getBackupFolder();
    let bookmarksBackupDir = new FileUtils.File(backupFolder);

    
    var htmlBackupFile = bookmarksBackupDir.clone();
    htmlBackupFile.append("bookmarks-2008-01-01.html");
    if (htmlBackupFile.exists())
      htmlBackupFile.remove(false);
    do_check_false(htmlBackupFile.exists());
    htmlBackupFile.create(Ci.nsILocalFile.NORMAL_FILE_TYPE, 0600);
    do_check_true(htmlBackupFile.exists());

    
    var jsonBackupFile = bookmarksBackupDir.clone();
    jsonBackupFile.append("bookmarks-2008-01-31.json");
    if (jsonBackupFile.exists())
      jsonBackupFile.remove(false);
    do_check_false(jsonBackupFile.exists());
    jsonBackupFile.create(Ci.nsILocalFile.NORMAL_FILE_TYPE, 0600);
    do_check_true(jsonBackupFile.exists());

    
    var backupFilename = PlacesBackups.getFilenameForDate();
    var rx = new RegExp("^" + backupFilename.replace(/\.json/, "") + "(_[0-9]+){0,1}\.json$");
    var files = bookmarksBackupDir.directoryEntries;
    var entry;
    while (files.hasMoreElements()) {
      entry = files.getNext().QueryInterface(Ci.nsIFile);
      if (entry.leafName.match(rx))
        entry.remove(false);
    }

    yield PlacesBackups.create(NUMBER_OF_BACKUPS);
    files = bookmarksBackupDir.directoryEntries;
    while (files.hasMoreElements()) {
      entry = files.getNext().QueryInterface(Ci.nsIFile);
      if (entry.leafName.match(rx))
        lastBackupFile = entry;
    }
    do_check_true(lastBackupFile.exists());

    
    do_check_false(htmlBackupFile.exists());
    do_check_false(jsonBackupFile.exists());
    do_check_true(lastBackupFile.exists());

    
    lastBackupFile.remove(false);
    do_check_false(lastBackupFile.exists());

    do_test_finished();
  });
}
