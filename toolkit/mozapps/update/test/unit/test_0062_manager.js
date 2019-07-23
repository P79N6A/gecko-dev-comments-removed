







































function run_test() {
  dump("Testing: resuming an update download in progress for the same " +
       "version of the application on startup - bug 485624\n");
  removeUpdateDirsAndFiles();
  setUpdateChannel();

  var patches, updates;

  patches = getLocalPatchString(null, null, null, null, null, null,
                                STATE_DOWNLOADING);
  updates = getLocalUpdateString(patches, null, null, "1.0", "1.0", null,
                                 null, null, null,
                                 URL_HOST + URL_PATH + "/empty.mar");
  writeUpdatesToXMLFile(getLocalUpdatesXMLString(updates), true);
  writeStatusFile(STATE_DOWNLOADING);

  writeUpdatesToXMLFile(getLocalUpdatesXMLString(""), false);

  standardInit();

  do_check_eq(gUpdateManager.updateCount, 1);
  do_check_eq(gUpdateManager.activeUpdate.state, STATE_DOWNLOADING);
  cleanUp();
}
