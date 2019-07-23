





function run_test() {
  removeUpdateDirsAndFiles();
  setUpdateChannel();

  writeUpdatesToXMLFile(getLocalUpdatesXMLString(""), false);
  var patches = getLocalPatchString(null, null, null, null, null, null,
                                    STATE_DOWNLOADING);
  var updates = getLocalUpdateString(patches);
  writeUpdatesToXMLFile(getLocalUpdatesXMLString(updates), true);
  writeStatusFile(STATE_NONE);

  standardInit();

  var dir = getUpdatesDir();
  dump("Testing: " + dir.path + " exists\n");
  dir.append("0");
  do_check_true(dir.exists());

  var statusFile = dir.clone();
  statusFile.append(FILE_UPDATE_STATUS);
  dump("Testing: " + statusFile.path + " does not exist\n");
  do_check_false(statusFile.exists());

  do_check_eq(gUpdateManager.activeUpdate, null);
  do_check_eq(gUpdateManager.updateCount, 0);

  cleanUp();
}
