







































var gNextRunFunc;
var gExpectedCount;

function run_test() {
  do_test_pending();
  removeUpdateDirsAndFiles();
  var pb = getPrefBranch();
  pb.setCharPref(PREF_APP_UPDATE_URL_OVERRIDE, URL_HOST + "update.xml");
  var defaults = pb.QueryInterface(AUS_Ci.nsIPrefService).getDefaultBranch(null);
  defaults.setCharPref("app.update.channel", "bogus_channel");
  
  overrideXHR(callHandleEvent);
  startAUS();
  startUpdateChecker();
  do_timeout(0, run_test_pt1);
}

function end_test() {
  do_test_finished();
  cleanUp();
}


function run_test_helper_pt1(aMsg, aExpectedCount, aNextRunFunc) {
  gUpdates = null;
  gUpdateCount = null;
  gCheckFunc = check_test_helper_pt1;
  gNextRunFunc = aNextRunFunc;
  gExpectedCount = aExpectedCount;
  dump("Testing: " + aMsg + "\n");
  gUpdateChecker.checkForUpdates(updateCheckListener, true);
}

function check_test_helper_pt1() {
  do_check_eq(gUpdateCount, gExpectedCount);
  gNextRunFunc();
}



function callHandleEvent() {
  gXHR.status = 400;
  gXHR.responseText = gResponseBody;
  try {
    var parser = AUS_Cc["@mozilla.org/xmlextras/domparser;1"].
                 createInstance(AUS_Ci.nsIDOMParser);
    gXHR.responseXML = parser.parseFromString(gResponseBody, "application/xml");
  }
  catch(e) {
    gXHR.responseXML = null;
  }
  var e = { target: gXHR };
  gXHR.onload.handleEvent(e);
}


function run_test_pt1() {
  run_test_helper_pt1("run_test_pt1 - update xml not available",
                      null, run_test_pt2);
}


function run_test_pt2() {
  dump("Testing: run_test_pt2 - one update available and the update's " +
       "property values\n");
  gUpdates = null;
  gUpdateCount = null;
  gCheckFunc = check_test_pt2;
  var patches = getRemotePatchString("complete", "http://complete/", "SHA1",
                                     "98db9dad8e1d80eda7e1170d0187d6f53e477059",
                                     "9856459");
  patches += getRemotePatchString("partial", "http://partial/", "SHA1",
                                  "e6678ca40ae7582316acdeddf3c133c9c8577de4",
                                  "1316138");
  var updates = getRemoteUpdateString(patches, "XPCShell App Update Test",
                                      "minor", "1.1a1pre", "2.1a1pre",
                                      "3.1a1pre", "20080811053724",
                                      "http://dummylicense/index.html",
                                      "http://dummydetails/index.html");
  gResponseBody = getRemoteUpdatesXMLString(updates);
  gUpdateChecker.checkForUpdates(updateCheckListener, true);
}

function check_test_pt2() {
  do_check_eq(gUpdateCount, 1);
  var bestUpdate = gAUS.selectUpdate(gUpdates, gUpdateCount);
  do_check_eq(bestUpdate.type, "minor");
  do_check_eq(bestUpdate.name, "XPCShell App Update Test");
  do_check_eq(bestUpdate.version, "1.1a1pre");
  do_check_eq(bestUpdate.platformVersion, "2.1a1pre");
  do_check_eq(bestUpdate.extensionVersion, "3.1a1pre");
  do_check_eq(bestUpdate.buildID, "20080811053724");
  do_check_eq(bestUpdate.licenseURL, "http://dummylicense/index.html");
  do_check_eq(bestUpdate.detailsURL, "http://dummydetails/index.html");
  do_check_eq(bestUpdate.serviceURL, URL_HOST + "update.xml?force=1");
  do_check_eq(bestUpdate.channel, "bogus_channel");
  do_check_false(bestUpdate.isCompleteUpdate);
  do_check_false(bestUpdate.isSecurityUpdate);
  do_check_eq(bestUpdate.installDate, 0);
  do_check_eq(bestUpdate.statusText, null);
  
  
  do_check_eq(bestUpdate.state, "");
  do_check_eq(bestUpdate.errorCode, 0);
  do_check_eq(bestUpdate.patchCount, 2);
  

  var type = "complete";
  var patch = bestUpdate.getPatchAt(0);
  do_check_eq(patch.type, type);
  do_check_eq(patch.URL, "http://" + type + "/");
  do_check_eq(patch.hashFunction, "SHA1");
  do_check_eq(patch.hashValue, "98db9dad8e1d80eda7e1170d0187d6f53e477059");
  do_check_eq(patch.size, 9856459);
  
  
  
  
  do_check_eq(typeof(patch.state), "string");
  do_check_eq(patch.state, STATE_NONE);
  do_check_false(patch.selected);
  

  type = "partial";
  patch = bestUpdate.getPatchAt(1);
  do_check_eq(patch.type, type);
  do_check_eq(patch.URL, "http://" + type + "/");
  do_check_eq(patch.hashFunction, "SHA1");
  do_check_eq(patch.hashValue, "e6678ca40ae7582316acdeddf3c133c9c8577de4");
  do_check_eq(patch.size, 1316138);
  do_check_eq(patch.state, STATE_NONE);
  do_check_false(patch.selected);
  

  run_test_pt3();
}


function run_test_pt3() {
  gResponseBody = "\n";
  run_test_helper_pt1("run_test_pt3 - empty update xml",
                      null, run_test_pt4);
}


function run_test_pt4() {
  gResponseBody = getRemoteUpdatesXMLString("");
  run_test_helper_pt1("run_test_pt4 - no updates available",
                      0, run_test_pt5);
}


function run_test_pt5() {
  var patches = getRemotePatchString("complete");
  patches += getRemotePatchString("partial");
  var updates = getRemoteUpdateString(patches);
  gResponseBody = getRemoteUpdatesXMLString(updates);
  run_test_helper_pt1("run_test_pt5 - one update available",
                      1, run_test_pt6);
}


function run_test_pt6() {
  var patches = getRemotePatchString("complete");
  patches += getRemotePatchString("partial");
  var updates = getRemoteUpdateString(patches);
  updates += getRemoteUpdateString(patches);
  updates += getRemoteUpdateString(patches);
  gResponseBody = getRemoteUpdatesXMLString(updates);
  run_test_helper_pt1("run_test_pt6 - three updates available",
                      3, run_test_pt7);
}



function run_test_pt7() {
  var patches = getRemotePatchString("complete", null, null, null, "0");
  patches += getRemotePatchString("partial", null, null, null, "0");
  var updates = getRemoteUpdateString(patches);
  gResponseBody = getRemoteUpdatesXMLString(updates);
  run_test_helper_pt1("run_test_pt7 - one update with complete and partial " +
                      "patches with size 0", 0, run_test_pt8);
}


function run_test_pt8() {
  var patches = getRemotePatchString("complete", null, null, null, "0");
  var updates = getRemoteUpdateString(patches);
  gResponseBody = getRemoteUpdatesXMLString(updates);
  run_test_helper_pt1("one update with complete patch with size 0",
                      0, run_test_pt9);
}


function run_test_pt9() {
  var patches = getRemotePatchString("partial", null, null, null, "0");
  var updates = getRemoteUpdateString(patches);
  gResponseBody = getRemoteUpdatesXMLString(updates);
  run_test_helper_pt1("one update with partial patch with size 0",
                      0, run_test_pt10);
}


function run_test_pt10() {
  var patches = getRemotePatchString("complete", "http://complete/", "SHA1",
                                     "98db9dad8e1d80eda7e1170d0187d6f53e477059",
                                     "9856459");
  patches += getRemotePatchString("partial", "http://partial/", "SHA1",
                                  "e6678ca40ae7582316acdeddf3c133c9c8577de4",
                                  "1316138");
  var updates = getRemoteUpdateString(patches, "XPCShell App Update Test",
                                      "minor", "version 1.0pre", "2.0",
                                      "1.0pre", "20080811053724",
                                      "http://dummylicense/index.html",
                                      "http://dummydetails/index.html");
  updates += getRemoteUpdateString(patches, "XPCShell App Update Test",
                                   "minor", "version 1.0a", "3.0",
                                   "1.0a", "20080811053724",
                                   "http://dummylicense/index.html",
                                   "http://dummydetails/index.html");
  gResponseBody = getRemoteUpdatesXMLString(updates);
  run_test_helper_pt1("two updates older than the current version",
                      2, check_test_pt10);
}

function check_test_pt10() {
  var bestUpdate = gAUS.selectUpdate(gUpdates, gUpdateCount);
  do_check_eq(bestUpdate, null);
  run_test_pt11();
}


function run_test_pt11() {
  var patches = getRemotePatchString("complete", "http://complete/", "SHA1",
                                     "98db9dad8e1d80eda7e1170d0187d6f53e477059",
                                     "9856459");
  patches += getRemotePatchString("partial", "http://partial/", "SHA1",
                                  "e6678ca40ae7582316acdeddf3c133c9c8577de4",
                                  "1316138");
  var updates = getRemoteUpdateString(patches, "XPCShell App Update Test",
                                      "minor", "version 1.0", "3.0",
                                      "1.0", "20080811053724",
                                      "http://dummylicense/index.html",
                                      "http://dummydetails/index.html");
  gResponseBody = getRemoteUpdatesXMLString(updates);
  run_test_helper_pt1("one updates equal to the current version",
                      1, check_test_pt11);
}

function check_test_pt11() {
  var bestUpdate = gAUS.selectUpdate(gUpdates, gUpdateCount);
  do_check_neq(bestUpdate, null);
  do_check_eq(bestUpdate.version, "version 1.0");
  end_test();
}
