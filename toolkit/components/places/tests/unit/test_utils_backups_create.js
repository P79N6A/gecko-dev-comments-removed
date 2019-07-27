





 



const NUMBER_OF_BACKUPS = 10;

function run_test() {
  run_next_test();
}

add_task(function () {
  
  let dateObj = new Date();
  let dates = [];
  while (dates.length < NUMBER_OF_BACKUPS) {
    
    let randomDate = new Date(dateObj.getFullYear() - 1,
                              Math.floor(12 * Math.random()),
                              Math.floor(28 * Math.random()));
    if (dates.indexOf(randomDate.getTime()) == -1)
      dates.push(randomDate.getTime());
  }
  
  dates.sort();

  
  let backupFolderPath = yield PlacesBackups.getBackupFolder();
  let bookmarksBackupDir = new FileUtils.File(backupFolderPath);

  
  
  
  for (let i = dates.length - 1; i >= 0; i--) {
    let backupFilename = PlacesBackups.getFilenameForDate(new Date(dates[i]));
    let backupFile = bookmarksBackupDir.clone();
    backupFile.append(backupFilename);
    backupFile.create(Ci.nsIFile.NORMAL_FILE_TYPE, parseInt("0666", 8));
    do_print("Creating fake backup " + backupFile.leafName);
    if (!backupFile.exists())
      do_throw("Unable to create fake backup " + backupFile.leafName);
  }

  yield PlacesBackups.create(NUMBER_OF_BACKUPS);
  
  dates.push(dateObj.getTime());

  
  
  for (let i = 0; i < dates.length; i++) {
    let backupFilename;
    let shouldExist;
    let backupFile;
    if (i > 0) {
      let files = bookmarksBackupDir.directoryEntries;
      while (files.hasMoreElements()) {
        let entry = files.getNext().QueryInterface(Ci.nsIFile);
        if (PlacesBackups.filenamesRegex.test(entry.leafName)) {
          backupFilename = entry.leafName;
          backupFile = entry;
          break;
        }
      }
      shouldExist = true;
    }
    else {
      backupFilename = PlacesBackups.getFilenameForDate(new Date(dates[i]));
      backupFile = bookmarksBackupDir.clone();
      backupFile.append(backupFilename);
      shouldExist = false;
    }
    if (backupFile.exists() != shouldExist)
      do_throw("Backup should " + (shouldExist ? "" : "not") + " exist: " + backupFilename);
  }

  
  
  
  let files = bookmarksBackupDir.directoryEntries;
  while (files.hasMoreElements()) {
    let entry = files.getNext().QueryInterface(Ci.nsIFile);
    entry.remove(false);
  }
  do_check_false(bookmarksBackupDir.directoryEntries.hasMoreElements());
});
