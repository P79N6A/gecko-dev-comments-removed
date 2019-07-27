




function run_test() {
  setupTestCommon();

  logTestInfo("testing resuming an update download in progress for the same " +
              "version of the application on startup (Bug 485624)");

  let patches = getLocalPatchString(null, null, null, null, null, null,
                                    STATE_DOWNLOADING);
  let updates = getLocalUpdateString(patches, null, null, "1.0", "1.0");
  writeUpdatesToXMLFile(getLocalUpdatesXMLString(updates), true);
  writeStatusFile(STATE_DOWNLOADING);

  writeUpdatesToXMLFile(getLocalUpdatesXMLString(""), false);

  standardInit();

  if (IS_TOOLKIT_GONK) {
    
    
    do_check_eq(gUpdateManager.updateCount, 0);
  } else {
    do_check_eq(gUpdateManager.updateCount, 1);
  }
  do_check_eq(gUpdateManager.activeUpdate.state, STATE_DOWNLOADING);

  
  
  
  gAUS.pauseDownload();
  writeUpdatesToXMLFile(getLocalUpdatesXMLString(""), true);
  writeUpdatesToXMLFile(getLocalUpdatesXMLString(""), false);
  reloadUpdateManagerData();

  do_execute_soon(doTestFinish);
}
