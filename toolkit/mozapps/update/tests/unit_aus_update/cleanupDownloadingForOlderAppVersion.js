





function run_test() {
  setupTestCommon();

  logTestInfo("testing cleanup of an update download in progress for an " +
              "older version of the application on startup (Bug 485624)");

  let patches = getLocalPatchString(null, null, null, null, null, null,
                                    STATE_DOWNLOADING);
  let updates = getLocalUpdateString(patches, null, null, "version 0.9", "0.9");
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

  doTestFinish();
}
