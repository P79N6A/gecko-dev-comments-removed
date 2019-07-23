






































Components.utils.import("resource://gre/modules/utils.js");

function run_test() {
  let bookmarksBackupDir = PlacesUtils.backups.folder;
  
  let files = bookmarksBackupDir.directoryEntries;
  while (files.hasMoreElements()) {
    let entry = files.getNext().QueryInterface(Ci.nsIFile);
    entry.remove(false);
  }

  
  let dateObj = new Date();
  dateObj.setYear(dateObj.getFullYear() + 1);
  let name = PlacesUtils.backups.getFilenameForDate(dateObj);
  do_check_eq(name, "bookmarks-" + dateObj.toLocaleFormat("%Y-%m-%d") + ".json");
  let futureBackupFile = bookmarksBackupDir.clone();
  futureBackupFile.append(name);
  if (futureBackupFile.exists())
    futureBackupFile.remove(false);
  do_check_false(futureBackupFile.exists());
  futureBackupFile.create(Ci.nsILocalFile.NORMAL_FILE_TYPE, 0600);
  do_check_true(futureBackupFile.exists());

  do_check_eq(PlacesUtils.backups.entries.length, 0);

  PlacesUtils.backups.create();

  
  do_check_eq(PlacesUtils.backups.entries.length, 1);
  let mostRecentBackupFile = PlacesUtils.backups.getMostRecent();
  do_check_neq(mostRecentBackupFile, null);
  let todayName = PlacesUtils.backups.getFilenameForDate();
  do_check_eq(mostRecentBackupFile.leafName, todayName);

  
  do_check_false(futureBackupFile.exists());

  
  mostRecentBackupFile.remove(false);
  do_check_false(mostRecentBackupFile.exists());
}
