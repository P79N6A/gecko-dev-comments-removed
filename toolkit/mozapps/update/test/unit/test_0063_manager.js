







































function run_test() {
  dump("Testing: removing an active update for a channel that is not valid" +
       "due to switching channels - bug 486275\n");
  removeUpdateDirsAndFiles();
  var defaults = getPrefBranch().QueryInterface(AUS_Ci.nsIPrefService).
                 getDefaultBranch(null);
  defaults.setCharPref("app.update.channel", "original_channel");

  var patches = getLocalPatchString(null, null, null, null, null, null,
                                    STATE_FAILED);
  var updates = getLocalUpdateString(patches, "Existing", null, "3.0", "3.0",
                                     "3.0", null, null, null, null, null,
                                     getString("patchApplyFailure"));
  writeUpdatesToXMLFile(getLocalUpdatesXMLString(updates), false);

  patches = getLocalPatchString(null, null, null, null, null, null,
                                STATE_DOWNLOADING);
  updates = getLocalUpdateString(patches, null, null, "1.0", null, "1.0", null,
                                 null, null, URL_HOST + DIR_DATA + "/empty.mar");
  writeUpdatesToXMLFile(getLocalUpdatesXMLString(updates), true);
  writeStatusFile(STATE_DOWNLOADING);

  startAUS();
  startUpdateManager();

  do_check_eq(gUpdateManager.updateCount, 1);
  var update = gUpdateManager.getUpdateAt(0);
  do_check_eq(update.name, "Existing");

  do_check_eq(gUpdateManager.activeUpdate, null);
  
  
  var file = getCurrentProcessDir();
  file.append(FILE_UPDATE_ACTIVE);
  dump("Testing: verifying contents of " + FILE_UPDATE_ACTIVE + "\n");
  do_check_eq(readFile(file), getLocalUpdatesXMLString(""));
  cleanUp();
}
