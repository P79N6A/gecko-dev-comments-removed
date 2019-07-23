





































Components.utils.import("resource://gre/modules/utils.js");

const NUMBER_OF_BACKUPS = 1;

function run_test() {
  
  var bookmarksBackupDir = PlacesUtils.backups.folder;

  
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

  
  var backupFilename = PlacesUtils.backups.getFilenameForDate();
  var lastBackupFile = bookmarksBackupDir.clone();
  lastBackupFile.append(backupFilename);
  if (lastBackupFile.exists())
    lastBackupFile.remove(false);
  do_check_false(lastBackupFile.exists());
  PlacesUtils.backups.create(NUMBER_OF_BACKUPS);
  do_check_true(lastBackupFile.exists());

  
  do_check_false(htmlBackupFile.exists());
  do_check_false(jsonBackupFile.exists());
  do_check_true(lastBackupFile.exists());

  
  lastBackupFile.remove(false);
  do_check_false(lastBackupFile.exists());
}
