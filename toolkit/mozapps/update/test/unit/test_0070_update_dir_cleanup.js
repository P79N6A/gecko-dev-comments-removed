







































function run_test() {
  do_test_pending();
  do_register_cleanup(end_test);

  removeUpdateDirsAndFiles();
  setUpdateChannel();

  writeUpdatesToXMLFile(getLocalUpdatesXMLString(""), false);
  var patches = getLocalPatchString(null, null, null, null, null, null,
                                    STATE_PENDING);
  var updates = getLocalUpdateString(patches);
  writeUpdatesToXMLFile(getLocalUpdatesXMLString(updates), true);
  writeStatusFile(STATE_SUCCEEDED);

  var dir = getUpdatesDir();
  var log = dir.clone();
  log.append("0");
  log.append(FILE_UPDATE_LOG);
  writeFile(log, "Last Update Log");

  let channelchange = dir.clone();
  channelchange.append("0");
  channelchange.append(CHANNEL_CHANGE_FILE);
  channelchange.create(AUS_Ci.nsIFile.FILE_TYPE, PERMS_FILE);

  standardInit();

  logTestInfo("testing " + log.path + " shouldn't exist");
  do_check_false(log.exists());

  log = dir.clone();
  log.append(FILE_LAST_LOG);
  logTestInfo("testing " + log.path + " should exist");
  do_check_true(log.exists());

  logTestInfo("testing " + log.path + " contents");
  do_check_eq(readFile(log), "Last Update Log");

  log = dir.clone();
  log.append(FILE_BACKUP_LOG);
  logTestInfo("testing " + log.path + " shouldn't exist");
  do_check_false(log.exists());

  dir.append("0");
  logTestInfo("testing " + dir.path + " should exist (bug 512994)");
  do_check_true(dir.exists());

  logTestInfo("testing " + channelchange.path + " shouldn't exist");
  do_check_false(channelchange.exists());

  do_test_finished();
}

function end_test() {
  cleanUp();
}
