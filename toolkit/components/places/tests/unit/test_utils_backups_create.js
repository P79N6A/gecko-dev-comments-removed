





 



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

  
  var bookmarksBackupDir = PlacesBackups.folder;

  
  
  
  for (let i = dates.length - 1; i >= 0; i--) {
    let backupFilename;
    if (i > Math.floor(dates.length/2))
      backupFilename = PREFIX + dates[i] + SUFFIX;
    else
      backupFilename = LOCALIZED_PREFIX + dates[i] + SUFFIX;
    dump("creating: " + backupFilename + "\n");
    let backupFile = bookmarksBackupDir.clone();
    backupFile.append(backupFilename);
    backupFile.create(Ci.nsIFile.NORMAL_FILE_TYPE, 0666);
    do_check_true(backupFile.exists());
  }
 
  
  
  PlacesUtils.getFormattedString = function (aKey, aValue) {
    return LOCALIZED_PREFIX + aValue;
  }

  Task.spawn(function() {
    yield PlacesBackups.create(Math.floor(dates.length/2));
    
    dates.push(dateObj.toLocaleFormat("%Y-%m-%d"));

    
    for (var i = 0; i < dates.length; i++) {
      let backupFilename;
      let shouldExist;
      if (i > Math.floor(dates.length/2)) {
        backupFilename = PREFIX + dates[i] + SUFFIX;
        shouldExist = true;
      }
      else {
        backupFilename = LOCALIZED_PREFIX + dates[i] + SUFFIX;
        shouldExist = false;
      }
      var backupFile = bookmarksBackupDir.clone();
      backupFile.append(backupFilename);
      if (backupFile.exists() != shouldExist)
        do_throw("Backup should " + (shouldExist ? "" : "not") + " exist: " + backupFilename);
    }

    
    bookmarksBackupDir.remove(true);
    do_check_false(bookmarksBackupDir.exists());
    
    PlacesBackups.folder;

    do_test_finished();
  });
}
