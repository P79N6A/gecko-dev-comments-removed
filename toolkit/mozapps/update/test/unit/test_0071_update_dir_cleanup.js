







































function run_test() {
  removeUpdateDirsAndFiles();
  var defaults = getPrefBranch().QueryInterface(AUS_Ci.nsIPrefService).
                 getDefaultBranch(null);
  defaults.setCharPref("app.update.channel", "bogus_channel");

  writeUpdatesToXMLFile(getLocalUpdatesXMLString(""), false);
  var patches = getLocalPatchString(null, null, null, null, null, null,
                                    STATE_PENDING);
  var updates = getLocalUpdateString(patches);
  writeUpdatesToXMLFile(getLocalUpdatesXMLString(updates), true);
  writeStatusFile(STATE_SUCCEEDED);

  var dir = getUpdatesDir();
  var log = dir.clone();
  log.append(FILE_LAST_LOG);
  writeFile(log, "Backup Update Log");

  log = dir.clone();
  log.append(FILE_BACKUP_LOG);
  writeFile(log, "To Be Deleted Backup Update Log");

  log = dir.clone();
  log.append("0");
  log.append(FILE_UPDATE_LOG);
  writeFile(log, "Last Update Log");

  startAUS();

  dump("Testing: " + FILE_UPDATE_LOG + " doesn't exist\n");
  do_check_false(log.exists());

  dump("Testing: " + FILE_LAST_LOG + " exists\n");
  log = dir.clone();
  log.append(FILE_LAST_LOG);
  do_check_true(log.exists());

  dump("Testing: " + FILE_LAST_LOG + " contents\n");
  do_check_eq(readFile(log), "Last Update Log");

  dump("Testing: " + FILE_BACKUP_LOG + " exists\n");
  log = dir.clone();
  log.append(FILE_BACKUP_LOG);
  do_check_true(log.exists());

  dump("Testing: " + FILE_BACKUP_LOG + " contents (bug 470979)\n");
  do_check_eq(readFile(log), "Backup Update Log");

  dump("Testing: " + dir.path + " exists (bug 512994)\n");
  dir.append("0");
  do_check_true(dir.exists());

  cleanUp();
}
