







































function run_test() {
  dump("Testing: removing an active update for a channel that is not valid " +
       "due to switching channels - bug 486275\n");
  removeUpdateDirsAndFiles();
  setUpdateChannel("original_channel");

  var patches, update, update;

  patches = getLocalPatchString(null, null, null, null, null, null,
                                STATE_DOWNLOADING);
  updates = getLocalUpdateString(patches, null, null, "version 1.0", "1.0", null,
                                 null, null, null,
                                 URL_HOST + URL_PATH + "/empty.mar");
  writeUpdatesToXMLFile(getLocalUpdatesXMLString(updates), true);
  writeStatusFile(STATE_DOWNLOADING);

  patches = getLocalPatchString(null, null, null, null, null, null,
                                STATE_FAILED);
  updates = getLocalUpdateString(patches, null, "Existing", "version 3.0",
                                 "3.0", "3.0", null, null, null, null, null,
                                 getString("patchApplyFailure"));
  writeUpdatesToXMLFile(getLocalUpdatesXMLString(updates), false);

  standardInit();

  do_check_eq(gUpdateManager.updateCount, 1);
  update = gUpdateManager.getUpdateAt(0);
  do_check_eq(update.name, "Existing");

  do_check_eq(gUpdateManager.activeUpdate, null);
  
  
  file = getCurrentProcessDir();
  file.append(FILE_UPDATE_ACTIVE);
  dump("Testing: verifying contents of " + FILE_UPDATE_ACTIVE + "\n");
  do_check_eq(readFile(file), getLocalUpdatesXMLString(""));
  cleanUp();
}
