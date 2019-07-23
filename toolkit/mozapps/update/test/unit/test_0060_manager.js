







































function run_test() {
  dump("Testing: addition of a successful update to " + FILE_UPDATES_DB +
       " and verification of update properties\n");
  removeUpdateDirsAndFiles();
  var defaults = gPrefs.QueryInterface(AUS_Ci.nsIPrefService)
                   .getDefaultBranch(null);
  defaults.setCharPref("app.update.channel", "bogus_channel");

  var patches = getLocalPatchString(null, null, null, null, null, null,
                                    STATE_FAILED);
  var updates = getLocalUpdateString(patches, "Existing", null, "3.0", "3.0",
                                     "3.0", null, null, null, null, null,
                                     getString("patchApplyFailure"));
  writeUpdatesToXMLFile(getLocalUpdatesXMLString(updates), false);

  patches = getLocalPatchString(null, null, null, null, null, null,
                                STATE_PENDING);
  updates = getLocalUpdateString(patches, "New");
  writeUpdatesToXMLFile(getLocalUpdatesXMLString(updates), true);
  writeStatusFile(STATE_SUCCEEDED);

  startAUS();
  startUpdateManager();

  do_check_eq(gUpdateManager.activeUpdate, null);
  do_check_eq(gUpdateManager.updateCount, 2);

  var update = gUpdateManager.getUpdateAt(0);
  do_check_eq(update.state, STATE_SUCCEEDED);
  do_check_eq(update.name, "New");
  do_check_eq(update.type, "major");
  do_check_eq(update.version, "4.0");
  do_check_eq(update.platformVersion, "4.0");
  do_check_eq(update.extensionVersion, "4.0");
  do_check_eq(update.detailsURL, "http://dummydetails/");
  do_check_eq(update.licenseURL, "http://dummylicense/");
  do_check_eq(update.serviceURL, "http://dummyservice/");
  do_check_eq(update.installDate, "1238441400314");
  do_check_eq(update.statusText, getString("installSuccess"));
  do_check_eq(update.buildID, "20080811053724");
  do_check_true(update.isCompleteUpdate);
  do_check_eq(update.channel, "bogus_channel");

  var patch = update.selectedPatch;
  do_check_eq(patch.type, "complete");
  do_check_eq(patch.URL, "http://localhost:4444/data/empty.mar");
  do_check_eq(patch.hashFunction, "MD5");
  do_check_eq(patch.hashValue, "6232cd43a1c77e30191c53a329a3f99d");
  do_check_eq(patch.size, "775");
  do_check_true(patch.selected);
  do_check_eq(patch.state, STATE_SUCCEEDED);

  update = gUpdateManager.getUpdateAt(1);
  do_check_eq(update.state, STATE_FAILED);
  do_check_eq(update.name, "Existing");
  do_check_eq(update.type, "major");
  do_check_eq(update.version, "3.0");
  do_check_eq(update.platformVersion, "3.0");
  do_check_eq(update.extensionVersion, "3.0");
  do_check_eq(update.detailsURL, "http://dummydetails/");
  do_check_eq(update.licenseURL, "http://dummylicense/");
  do_check_eq(update.serviceURL, "http://dummyservice/");
  do_check_eq(update.installDate, "1238441400314");
  do_check_eq(update.statusText, getString("patchApplyFailure"));
  do_check_eq(update.buildID, "20080811053724");
  do_check_true(update.isCompleteUpdate);
  do_check_eq(update.channel, "bogus_channel");

  patch = update.selectedPatch;
  do_check_eq(patch.type, "complete");
  do_check_eq(patch.URL, "http://localhost:4444/data/empty.mar");
  do_check_eq(patch.hashFunction, "MD5");
  do_check_eq(patch.hashValue, "6232cd43a1c77e30191c53a329a3f99d");
  do_check_eq(patch.size, "775");
  do_check_true(patch.selected);
  do_check_eq(patch.state, STATE_FAILED);
}
