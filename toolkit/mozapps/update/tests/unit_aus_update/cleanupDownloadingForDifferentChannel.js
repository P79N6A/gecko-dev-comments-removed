






function run_test() {
  setupTestCommon();

  debugDump("testing removal of an active update for a channel that is not" +
            "valid due to switching channels (Bug 486275).");

  let patches = getLocalPatchString(null, null, null, null, null, null,
                                    STATE_DOWNLOADING);
  let updates = getLocalUpdateString(patches, null, null, "version 1.0", "1.0");
  writeUpdatesToXMLFile(getLocalUpdatesXMLString(updates), true);
  writeStatusFile(STATE_DOWNLOADING);

  patches = getLocalPatchString(null, null, null, null, null, null,
                                STATE_FAILED);
  updates = getLocalUpdateString(patches, null, "Existing", "version 3.0",
                                 "3.0", "3.0", null, null, null, null, null,
                                 getString("patchApplyFailure"));
  writeUpdatesToXMLFile(getLocalUpdatesXMLString(updates), false);

  setUpdateChannel("original_channel");

  standardInit();

  do_check_eq(gUpdateManager.updateCount, 1);
  let update = gUpdateManager.getUpdateAt(0);
  do_check_eq(update.name, "Existing");

  do_check_eq(gUpdateManager.activeUpdate, null);
  
  
  let file = getUpdatesXMLFile(true);
  debugDump("verifying contents of " + FILE_UPDATE_ACTIVE);
  do_check_eq(readFile(file), getLocalUpdatesXMLString(""));

  doTestFinish();
}
