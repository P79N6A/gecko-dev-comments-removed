




function run_test() {
  setupTestCommon();

  debugDump("testing resuming an update download in progress for the same " +
            "version of the application on startup (Bug 485624)");

  let patches = getLocalPatchString(null, null, null, null, null, null,
                                    STATE_DOWNLOADING);
  let updates = getLocalUpdateString(patches, null, null, "1.0", "1.0");
  writeUpdatesToXMLFile(getLocalUpdatesXMLString(updates), true);
  writeStatusFile(STATE_DOWNLOADING);

  writeUpdatesToXMLFile(getLocalUpdatesXMLString(""), false);

  standardInit();

  if (IS_TOOLKIT_GONK) {
    
    
    Assert.equal(gUpdateManager.updateCount, 0,
                 "the update manager updateCount attribute" + MSG_SHOULD_EQUAL);
  } else {
    Assert.equal(gUpdateManager.updateCount, 1,
                 "the update manager updateCount attribute" + MSG_SHOULD_EQUAL);
  }
  Assert.equal(gUpdateManager.activeUpdate.state, STATE_DOWNLOADING,
               "the update manager activeUpdate state attribute" +
               MSG_SHOULD_EQUAL);

  
  
  
  gAUS.pauseDownload();
  writeUpdatesToXMLFile(getLocalUpdatesXMLString(""), true);
  writeUpdatesToXMLFile(getLocalUpdatesXMLString(""), false);
  reloadUpdateManagerData();

  do_execute_soon(doTestFinish);
}
