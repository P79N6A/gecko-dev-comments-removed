








add_task(function check_max_backups_is_respected() {
  
  let backupFolder = yield PlacesBackups.getBackupFolder();

  
  let oldJsonPath = OS.Path.join(backupFolder, "bookmarks-2008-01-01.json");
  let oldJsonFile = yield OS.File.open(oldJsonPath, { truncate: true });
  oldJsonFile.close();
  do_check_true(yield OS.File.exists(oldJsonPath));

  let jsonPath = OS.Path.join(backupFolder, "bookmarks-2008-01-31.json");
  let jsonFile = yield OS.File.open(jsonPath, { truncate: true });
  jsonFile.close();
  do_check_true(yield OS.File.exists(jsonPath));

  
  
  yield PlacesBackups.create(2);
  let backupFilename = PlacesBackups.getFilenameForDate();

  let count = 0;
  let lastBackupPath = null;
  let iterator = new OS.File.DirectoryIterator(backupFolder);
  try {
    yield iterator.forEach(aEntry => {
      count++;
      if (PlacesBackups.filenamesRegex.test(aEntry.name))
        lastBackupPath = aEntry.path;
    });
  } finally {
    iterator.close();
  }

  do_check_eq(count, 2);
  do_check_neq(lastBackupPath, null);
  do_check_false(yield OS.File.exists(oldJsonPath));
  do_check_true(yield OS.File.exists(jsonPath));
});

add_task(function check_max_backups_greater_than_backups() {
  
  let backupFolder = yield PlacesBackups.getBackupFolder();

  
  
  yield PlacesBackups.create(3);
  let backupFilename = PlacesBackups.getFilenameForDate();

  let count = 0;
  let lastBackupPath = null;
  let iterator = new OS.File.DirectoryIterator(backupFolder);
  try {
    yield iterator.forEach(aEntry => {
      count++;
      if (PlacesBackups.filenamesRegex.test(aEntry.name))
        lastBackupPath = aEntry.path;
    });
  } finally {
    iterator.close();
  }
  do_check_eq(count, 2);
  do_check_neq(lastBackupPath, null);
});

add_task(function check_max_backups_null() {
  
  let backupFolder = yield PlacesBackups.getBackupFolder();

  
  
  
  yield PlacesBackups.create(null);
  let backupFilename = PlacesBackups.getFilenameForDate();

  let count = 0;
  let lastBackupPath = null;
  let iterator = new OS.File.DirectoryIterator(backupFolder);
  try {
    yield iterator.forEach(aEntry => {
      count++;
      if (PlacesBackups.filenamesRegex.test(aEntry.name))
        lastBackupPath = aEntry.path;
    });
  } finally {
    iterator.close();
  }
  do_check_eq(count, 2);
  do_check_neq(lastBackupPath, null);
});

add_task(function check_max_backups_undefined() {
  
  let backupFolder = yield PlacesBackups.getBackupFolder();

  
  
  
  yield PlacesBackups.create();
  let backupFilename = PlacesBackups.getFilenameForDate();

  let count = 0;
  let lastBackupPath = null;
  let iterator = new OS.File.DirectoryIterator(backupFolder);
  try {
    yield iterator.forEach(aEntry => {
      count++;
      if (PlacesBackups.filenamesRegex.test(aEntry.name))
        lastBackupPath = aEntry.path;
    });
  } finally {
    iterator.close();
  }
  do_check_eq(count, 2);
  do_check_neq(lastBackupPath, null);
});

function run_test() {
  run_next_test();
}
