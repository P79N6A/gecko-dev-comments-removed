







































function run_test() {
  dump("Testing: removal of an update download in progress for an older " +
       "version of the application on startup - bug 485624\n");
  removeUpdateDirsAndFiles();
  var defaults = gPrefs.QueryInterface(AUS_Ci.nsIPrefService)
                   .getDefaultBranch(null);
  defaults.setCharPref("app.update.channel", "bogus_channel");

  writeUpdatesToXMLFile(getLocalUpdatesXMLString(""), false);

  patches = getLocalPatchString(null, null, null, null, null, null,
                                STATE_DOWNLOADING);
  updates = getLocalUpdateString(patches, null, null, "0.9", null, "0.9");
  writeUpdatesToXMLFile(getLocalUpdatesXMLString(updates), true);
  writeStatusFile(STATE_DOWNLOADING);

  startAUS();
  startUpdateManager();

  do_check_eq(gUpdateManager.activeUpdate, null);
  do_check_eq(gUpdateManager.updateCount, 0);
}
