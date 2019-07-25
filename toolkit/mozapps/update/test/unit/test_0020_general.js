







































var gNextRunFunc;
var gExpectedCount;

function run_test() {
  do_test_pending();
  do_register_cleanup(end_test);
  removeUpdateDirsAndFiles();
  setUpdateURLOverride();
  setUpdateChannel();
  
  overrideXHR(callHandleEvent);
  standardInit();
  do_execute_soon(run_test_pt01);
}

function end_test() {
  cleanUp();
}


function run_test_helper_pt1(aMsg, aExpectedCount, aNextRunFunc) {
  gUpdates = null;
  gUpdateCount = null;
  gCheckFunc = check_test_helper_pt1;
  gNextRunFunc = aNextRunFunc;
  gExpectedCount = aExpectedCount;
  logTestInfo(aMsg, Components.stack.caller);
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
  gXHR.onload(e);
}


function run_test_pt01() {
  run_test_helper_pt1("testing update xml not available",
                      null, run_test_pt02);
}


function run_test_pt02() {
  logTestInfo("testing one update available and the update's property values");
  gUpdates = null;
  gUpdateCount = null;
  gCheckFunc = check_test_pt02;
  var patches = getRemotePatchString("complete", "http://complete/", "SHA1",
                                     "98db9dad8e1d80eda7e1170d0187d6f53e477059",
                                     "9856459");
  patches += getRemotePatchString("partial", "http://partial/", "SHA1",
                                  "e6678ca40ae7582316acdeddf3c133c9c8577de4",
                                  "1316138");
  var updates = getRemoteUpdateString(patches, "minor", "Minor Test",
                                      "version 2.1a1pre", "2.1a1pre",
                                      "3.1a1pre", "20080811053724",
                                      "http://details/",
                                      "http://billboard/",
                                      "http://license/", "true",
                                      "true", "true", "4.1a1pre", "5.1a1pre",
                                      "custom1_attr=\"custom1 value\"",
                                      "custom2_attr=\"custom2 value\"");
  gResponseBody = getRemoteUpdatesXMLString(updates);
  gUpdateChecker.checkForUpdates(updateCheckListener, true);
}

function check_test_pt02() {
  
  
  


    
    








  do_check_eq(gUpdateCount, 1);
  var bestUpdate = gAUS.selectUpdate(gUpdates, gUpdateCount).QueryInterface(AUS_Ci.nsIPropertyBag);
  do_check_eq(bestUpdate.type, "minor");
  do_check_eq(bestUpdate.name, "Minor Test");
  do_check_eq(bestUpdate.displayVersion, "version 2.1a1pre");
  do_check_eq(bestUpdate.appVersion, "2.1a1pre");
  do_check_eq(bestUpdate.platformVersion, "3.1a1pre");
  do_check_eq(bestUpdate.buildID, "20080811053724");
  do_check_eq(bestUpdate.detailsURL, "http://details/");
  do_check_eq(bestUpdate.billboardURL, "http://billboard/");
  do_check_eq(bestUpdate.licenseURL, "http://license/");
  do_check_true(bestUpdate.showPrompt);
  do_check_true(bestUpdate.showNeverForVersion);
  do_check_true(bestUpdate.showSurvey);
  do_check_eq(bestUpdate.serviceURL, URL_HOST + "update.xml?force=1");
  do_check_eq(bestUpdate.channel, "test_channel");
  do_check_false(bestUpdate.isCompleteUpdate);
  do_check_false(bestUpdate.isSecurityUpdate);
  
  do_check_true((Date.now() - bestUpdate.installDate) < 10000);
  do_check_eq(bestUpdate.statusText, null);
  
  
  do_check_eq(bestUpdate.state, "");
  do_check_eq(bestUpdate.errorCode, 0);
  do_check_eq(bestUpdate.patchCount, 2);
  

  do_check_eq(bestUpdate.getProperty("custom1_attr"), "custom1 value");
  do_check_eq(bestUpdate.getProperty("custom2_attr"), "custom2 value");

  var patch = bestUpdate.getPatchAt(0);
  do_check_eq(patch.type, "complete");
  do_check_eq(patch.URL, "http://complete/");
  do_check_eq(patch.hashFunction, "SHA1");
  do_check_eq(patch.hashValue, "98db9dad8e1d80eda7e1170d0187d6f53e477059");
  do_check_eq(patch.size, 9856459);
  
  
  
  
  do_check_eq(typeof(patch.state), "string");
  do_check_eq(patch.state, STATE_NONE);
  do_check_false(patch.selected);
  

  patch = bestUpdate.getPatchAt(1);
  do_check_eq(patch.type, "partial");
  do_check_eq(patch.URL, "http://partial/");
  do_check_eq(patch.hashFunction, "SHA1");
  do_check_eq(patch.hashValue, "e6678ca40ae7582316acdeddf3c133c9c8577de4");
  do_check_eq(patch.size, 1316138);
  do_check_eq(patch.state, STATE_NONE);
  do_check_false(patch.selected);
  

  run_test_pt03();
}


function run_test_pt03() {
  logTestInfo("testing one update available and the update's property values " +
              "with the format prior to bug 530872");
  gUpdates = null;
  gUpdateCount = null;
  gCheckFunc = check_test_pt03;
  var patches = getRemotePatchString("complete", "http://complete/", "SHA1",
                                     "98db9dad8e1d80eda7e1170d0187d6f53e477059",
                                     "9856459");
  var updates = getRemoteUpdateString(patches, "major", "Major Test",
                                      null, null,
                                      "5.1a1pre", "20080811053724",
                                      "http://details/",
                                      null, null, null, null, null,
                                      "version 4.1a1pre", "4.1a1pre");
  gResponseBody = getRemoteUpdatesXMLString(updates);
  gUpdateChecker.checkForUpdates(updateCheckListener, true);
}

function check_test_pt03() {
  do_check_eq(gUpdateCount, 1);
  var bestUpdate = gAUS.selectUpdate(gUpdates, gUpdateCount);
  do_check_eq(bestUpdate.type, "major");
  do_check_eq(bestUpdate.name, "Major Test");
  do_check_eq(bestUpdate.displayVersion, "version 4.1a1pre");
  do_check_eq(bestUpdate.appVersion, "4.1a1pre");
  do_check_eq(bestUpdate.platformVersion, "5.1a1pre");
  do_check_eq(bestUpdate.buildID, "20080811053724");
  do_check_eq(bestUpdate.detailsURL, "http://details/");
  do_check_eq(bestUpdate.billboardURL, "http://details/");
  do_check_eq(bestUpdate.licenseURL, null);
  do_check_true(bestUpdate.showPrompt);
  do_check_true(bestUpdate.showNeverForVersion);
  do_check_false(bestUpdate.showSurvey);
  do_check_eq(bestUpdate.serviceURL, URL_HOST + "update.xml?force=1");
  do_check_eq(bestUpdate.channel, "test_channel");
  do_check_false(bestUpdate.isCompleteUpdate);
  do_check_false(bestUpdate.isSecurityUpdate);
  
  do_check_true((Date.now() - bestUpdate.installDate) < 10000);
  do_check_eq(bestUpdate.statusText, null);
  
  
  do_check_eq(bestUpdate.state, "");
  do_check_eq(bestUpdate.errorCode, 0);
  do_check_eq(bestUpdate.patchCount, 1);
  

  var patch = bestUpdate.getPatchAt(0);
  do_check_eq(patch.type, "complete");
  do_check_eq(patch.URL, "http://complete/");
  do_check_eq(patch.hashFunction, "SHA1");
  do_check_eq(patch.hashValue, "98db9dad8e1d80eda7e1170d0187d6f53e477059");
  do_check_eq(patch.size, 9856459);
  
  
  
  
  do_check_eq(typeof(patch.state), "string");
  do_check_eq(patch.state, STATE_NONE);
  do_check_false(patch.selected);
  

  run_test_pt04();
}


function run_test_pt04() {
  gResponseBody = "\n";
  run_test_helper_pt1("testing empty update xml",
                      null, run_test_pt05);
}


function run_test_pt05() {
  gResponseBody = getRemoteUpdatesXMLString("");
  run_test_helper_pt1("testing no updates available",
                      0, run_test_pt06);
}


function run_test_pt06() {
  var patches = getRemotePatchString("complete");
  patches += getRemotePatchString("partial");
  var updates = getRemoteUpdateString(patches);
  gResponseBody = getRemoteUpdatesXMLString(updates);
  run_test_helper_pt1("testing one update available",
                      1, run_test_pt07);
}


function run_test_pt07() {
  var patches = getRemotePatchString("complete");
  patches += getRemotePatchString("partial");
  var updates = getRemoteUpdateString(patches);
  updates += getRemoteUpdateString(patches);
  updates += getRemoteUpdateString(patches);
  gResponseBody = getRemoteUpdatesXMLString(updates);
  run_test_helper_pt1("testing three updates available",
                      3, run_test_pt08);
}



function run_test_pt08() {
  var patches = getRemotePatchString("complete", null, null, null, "0");
  patches += getRemotePatchString("partial", null, null, null, "0");
  var updates = getRemoteUpdateString(patches);
  gResponseBody = getRemoteUpdatesXMLString(updates);
  run_test_helper_pt1("testing one update with complete and partial " +
                      "patches with size 0", 0, run_test_pt09);
}


function run_test_pt09() {
  var patches = getRemotePatchString("complete", null, null, null, "0");
  var updates = getRemoteUpdateString(patches);
  gResponseBody = getRemoteUpdatesXMLString(updates);
  run_test_helper_pt1("testing one update with complete patch with size 0",
                      0, run_test_pt10);
}


function run_test_pt10() {
  var patches = getRemotePatchString("partial", null, null, null, "0");
  var updates = getRemoteUpdateString(patches);
  gResponseBody = getRemoteUpdatesXMLString(updates);
  run_test_helper_pt1("testing one update with partial patch with size 0",
                      0, run_test_pt11);
}


function run_test_pt11() {
  var patches = getRemotePatchString("complete");
  patches += getRemotePatchString("partial");
  var updates = getRemoteUpdateString(patches, "minor", null, null, "1.0pre");
  updates += getRemoteUpdateString(patches, "minor", null, null, "1.0a");
  gResponseBody = getRemoteUpdatesXMLString(updates);
  run_test_helper_pt1("testing two updates older than the current version",
                      2, check_test_pt11);
}

function check_test_pt11() {
  var bestUpdate = gAUS.selectUpdate(gUpdates, gUpdateCount);
  do_check_eq(bestUpdate, null);
  run_test_pt12();
}


function run_test_pt12() {
  var patches = getRemotePatchString("complete");
  patches += getRemotePatchString("partial");
  var updates = getRemoteUpdateString(patches, "minor", null, "version 1.0");
  gResponseBody = getRemoteUpdatesXMLString(updates);
  run_test_helper_pt1("testing one update equal to the current version",
                      1, check_test_pt12);
}

function check_test_pt12() {
  var bestUpdate = gAUS.selectUpdate(gUpdates, gUpdateCount);
  do_check_neq(bestUpdate, null);
  do_check_eq(bestUpdate.displayVersion, "version 1.0");
  do_test_finished();
}
