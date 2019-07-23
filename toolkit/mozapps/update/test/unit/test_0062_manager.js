







































function run_test() {
  dump("Testing: resuming an update download in progress for the same " +
       "version of the application on startup - bug 485624\n");
  removeUpdateDirsAndFiles();
  var defaults = getPrefBranch().QueryInterface(AUS_Ci.nsIPrefService).
                 getDefaultBranch(null);
  defaults.setCharPref("app.update.channel", "bogus_channel");

  writeUpdatesToXMLFile(getLocalUpdatesXMLString(""), false);

  var patches = getLocalPatchString(null, null, null, null, null, null,
                                    STATE_DOWNLOADING);
  var updates = getLocalUpdateString(patches, null, null, "1.0", null, "1.0",
                                     null, null, null,
                                     URL_HOST + DIR_DATA + "/empty.mar");
  writeUpdatesToXMLFile(getLocalUpdatesXMLString(updates), true);
  writeStatusFile(STATE_DOWNLOADING);

  startAUS();
  startUpdateManager();

  do_check_eq(gUpdateManager.updateCount, 1);
  do_check_eq(gUpdateManager.activeUpdate.state, STATE_DOWNLOADING);
  cleanUp();
}
