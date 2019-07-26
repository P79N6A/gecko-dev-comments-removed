





 



const PREFIX = "bookmarks-";

const LOCALIZED_PREFIX = "segnalibri-";
const SUFFIX = ".json";
const NUMBER_OF_BACKUPS = 10;

function run_test() {
  do_test_pending();

  
  var dateObj = new Date();
  var dates = [];
  while (dates.length < NUMBER_OF_BACKUPS) {
    
    let randomDate = new Date(dateObj.getFullYear() - 1,
                              Math.floor(12 * Math.random()),
                              Math.floor(28 * Math.random()));
    let dateString = randomDate.toLocaleFormat("%Y-%m-%d");
    if (dates.indexOf(dateString) == -1)
      dates.push(dateString);
  }
  
  dates.sort();

  Task.spawn(function() {
    
    let backupFolderPath = yield PlacesBackups.getBackupFolder();
    let bookmarksBackupDir = new FileUtils.File(backupFolderPath);

    
    
    
    for (let i = dates.length - 1; i >= 0; i--) {
      let backupFilename;
      if (i > Math.floor(dates.length/2))
        backupFilename = PREFIX + dates[i] + SUFFIX;
      else
        backupFilename = LOCALIZED_PREFIX + dates[i] + SUFFIX;
      let backupFile = bookmarksBackupDir.clone();
      backupFile.append(backupFilename);
      backupFile.create(Ci.nsIFile.NORMAL_FILE_TYPE, 0666);
      do_check_true(backupFile.exists());
    }

    
    
    PlacesUtils.getFormattedString = function (aKey, aValue) {
      return LOCALIZED_PREFIX + aValue;
    }

    yield PlacesBackups.create(Math.floor(dates.length/2));
    
    dates.push(dateObj.toLocaleFormat("%Y-%m-%d"));

    
    for (var i = 0; i < dates.length; i++) {
      let backupFilename;
      let shouldExist;
      let backupFile;
      if (i > Math.floor(dates.length/2)) {
        let files = bookmarksBackupDir.directoryEntries;
        let rx = new RegExp("^" + PREFIX + dates[i] + "(_[0-9]+){0,1}" + SUFFIX + "$");
        while (files.hasMoreElements()) {
          let entry = files.getNext().QueryInterface(Ci.nsIFile);
          if (entry.leafName.match(rx)) {
            backupFilename = entry.leafName;
            backupFile = entry;
            break;
          }
        }
        shouldExist = true;
      }
      else {
        backupFilename = LOCALIZED_PREFIX + dates[i] + SUFFIX;
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

    do_test_finished();
  });
}
