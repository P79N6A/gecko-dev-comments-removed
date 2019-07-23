







































function run_test() {
  dump("Testing: removal of an update download in progress for the same " +
       "version of the application with the same application build id on " +
       "startup - bug 536547\n");
  removeUpdateDirsAndFiles();
  var defaults = getPrefBranch().QueryInterface(AUS_Ci.nsIPrefService).
                 getDefaultBranch(null);
  defaults.setCharPref("app.update.channel", "bogus_channel");

  writeUpdatesToXMLFile(getLocalUpdatesXMLString(""), false);

  var patches = getLocalPatchString(null, null, null, null, null, null,
                                    STATE_DOWNLOADING);
  var updates = getLocalUpdateString(patches, null, null, "1.0", null, "1.0",
                                     "2007010101");
  writeUpdatesToXMLFile(getLocalUpdatesXMLString(updates), true);
  writeStatusFile(STATE_DOWNLOADING);

  startAUS();
  startUpdateManager();

  do_check_eq(gUpdateManager.activeUpdate, null);
  do_check_eq(gUpdateManager.updateCount, 0);
  cleanUp();
}
