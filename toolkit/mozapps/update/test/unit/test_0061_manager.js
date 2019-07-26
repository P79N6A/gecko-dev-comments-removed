






function run_test() {
  do_test_pending();
  do_register_cleanup(end_test);

  logTestInfo("testing removal of an update download in progress for an " +
              "older version of the application on startup (bug 485624)");
  removeUpdateDirsAndFiles();

  var patches, updates;

  patches = getLocalPatchString(null, null, null, null, null, null,
                                STATE_DOWNLOADING);
  updates = getLocalUpdateString(patches, null, null, "version 0.9", "0.9");
  writeUpdatesToXMLFile(getLocalUpdatesXMLString(updates), true);
  writeStatusFile(STATE_DOWNLOADING);

  writeUpdatesToXMLFile(getLocalUpdatesXMLString(""), false);

  standardInit();

  if (IS_TOOLKIT_GONK) {
    
    
    
    do_check_neq(gUpdateManager.activeUpdate, null);
  } else {
    do_check_eq(gUpdateManager.activeUpdate, null);
  }
  do_check_eq(gUpdateManager.updateCount, 0);

  do_test_finished();
}

function end_test() {
  cleanUp();
}
