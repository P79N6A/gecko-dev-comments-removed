




var gNextRunFunc;
var gExpectedCount;

function run_test() {
  setupTestCommon();

  debugDump("testing remote update xml attributes");

  setUpdateURLOverride();
  setUpdateChannel("test_channel");
  
  overrideXHR(callHandleEvent);
  standardInit();
  do_execute_soon(run_test_pt01);
}


function run_test_helper_pt1(aMsg, aExpectedCount, aNextRunFunc) {
  gUpdates = null;
  gUpdateCount = null;
  gCheckFunc = check_test_helper_pt1;
  gNextRunFunc = aNextRunFunc;
  gExpectedCount = aExpectedCount;
  debugDump(aMsg, Components.stack.caller);
  gUpdateChecker.checkForUpdates(updateCheckListener, true);
}

function check_test_helper_pt1() {
  Assert.equal(gUpdateCount, gExpectedCount,
               "the update count" + MSG_SHOULD_EQUAL);
  gNextRunFunc();
}



function callHandleEvent(aXHR) {
  aXHR.status = 400;
  aXHR.responseText = gResponseBody;
  try {
    if (gResponseBody) {
      let parser = Cc["@mozilla.org/xmlextras/domparser;1"].
                   createInstance(Ci.nsIDOMParser);
      aXHR.responseXML = parser.parseFromString(gResponseBody, "application/xml");
    }
  } catch (e) {
    aXHR.responseXML = null;
  }
  let e = { target: aXHR };
  aXHR.onload(e);
}


function run_test_pt01() {
  run_test_helper_pt1("testing update xml not available",
                      null, run_test_pt02);
}


function run_test_pt02() {
  debugDump("testing one update available and the update's property values");
  gUpdates = null;
  gUpdateCount = null;
  gCheckFunc = check_test_pt02;
  let patches = getRemotePatchString("complete", "http://complete/", "SHA1",
                                     "98db9dad8e1d80eda7e1170d0187d6f53e477059",
                                     "9856459");
  patches += getRemotePatchString("partial", "http://partial/", "SHA1",
                                  "e6678ca40ae7582316acdeddf3c133c9c8577de4",
                                  "1316138");
  let updates = getRemoteUpdateString(patches, "minor", "Minor Test",
                                      "version 2.1a1pre", "2.1a1pre",
                                      "3.1a1pre", "20080811053724",
                                      "http://details/",
                                      "http://billboard/",
                                      "http://license/", "true",
                                      "true", "345600", "true", "4.1a1pre",
                                      "5.1a1pre",
                                      "custom1_attr=\"custom1 value\"",
                                      "custom2_attr=\"custom2 value\"");
  gResponseBody = getRemoteUpdatesXMLString(updates);
  gUpdateChecker.checkForUpdates(updateCheckListener, true);
}

function check_test_pt02() {
  
  
  


    
    







  Assert.equal(gUpdateCount, 1,
               "the update count" + MSG_SHOULD_EQUAL);
  let bestUpdate = gAUS.selectUpdate(gUpdates, gUpdateCount).QueryInterface(Ci.nsIPropertyBag);
  Assert.equal(bestUpdate.type, "minor",
               "the update type attribute" + MSG_SHOULD_EQUAL);
  Assert.equal(bestUpdate.name, "Minor Test",
               "the update name attribute" + MSG_SHOULD_EQUAL);
  Assert.equal(bestUpdate.displayVersion, "version 2.1a1pre",
               "the update displayVersion attribute" + MSG_SHOULD_EQUAL);
  Assert.equal(bestUpdate.appVersion, "2.1a1pre",
               "the update appVersion attribute" + MSG_SHOULD_EQUAL);
  Assert.equal(bestUpdate.platformVersion, "3.1a1pre",
               "the update platformVersion attribute" + MSG_SHOULD_EQUAL);
  Assert.equal(bestUpdate.buildID, "20080811053724",
               "the update buildID attribute" + MSG_SHOULD_EQUAL);
  Assert.equal(bestUpdate.detailsURL, "http://details/",
               "the update detailsURL attribute" + MSG_SHOULD_EQUAL);
  Assert.equal(bestUpdate.billboardURL, "http://billboard/",
               "the update billboardURL attribute" + MSG_SHOULD_EQUAL);
  Assert.equal(bestUpdate.licenseURL, "http://license/",
               "the update licenseURL attribute" + MSG_SHOULD_EQUAL);
  Assert.ok(bestUpdate.showPrompt,
            "the update showPrompt attribute" + MSG_SHOULD_EQUAL);
  Assert.ok(bestUpdate.showNeverForVersion,
            "the update showNeverForVersion attribute" + MSG_SHOULD_EQUAL);
  Assert.equal(bestUpdate.promptWaitTime, "345600",
               "the update promptWaitTime attribute" + MSG_SHOULD_EQUAL);
  Assert.equal(bestUpdate.serviceURL, URL_HOST + "/update.xml?force=1",
               "the update serviceURL attribute" + MSG_SHOULD_EQUAL);
  Assert.equal(bestUpdate.channel, "test_channel",
               "the update channel attribute" + MSG_SHOULD_EQUAL);
  Assert.ok(!bestUpdate.isCompleteUpdate,
            "the update isCompleteUpdate attribute" + MSG_SHOULD_EQUAL);
  Assert.ok(!bestUpdate.isSecurityUpdate,
            "the update isSecurityUpdate attribute" + MSG_SHOULD_EQUAL);
  
  Assert.ok((Date.now() - bestUpdate.installDate) < 10000,
            "the update installDate attribute should be within 10 seconds of " +
            "the current time");
  Assert.ok(!bestUpdate.statusText,
            "the update statusText attribute" + MSG_SHOULD_EQUAL);
  
  
  Assert.equal(bestUpdate.state, "",
               "the update state attribute" + MSG_SHOULD_EQUAL);
  Assert.equal(bestUpdate.errorCode, 0,
               "the update errorCode attribute" + MSG_SHOULD_EQUAL);
  Assert.equal(bestUpdate.patchCount, 2,
               "the update patchCount attribute" + MSG_SHOULD_EQUAL);
  

  Assert.equal(bestUpdate.getProperty("custom1_attr"), "custom1 value",
               "the update custom1_attr property" + MSG_SHOULD_EQUAL);
  Assert.equal(bestUpdate.getProperty("custom2_attr"), "custom2 value",
               "the update custom2_attr property" + MSG_SHOULD_EQUAL);

  let patch = bestUpdate.getPatchAt(0);
  Assert.equal(patch.type, "complete",
               "the update patch type attribute" + MSG_SHOULD_EQUAL);
  Assert.equal(patch.URL, "http://complete/",
               "the update patch URL attribute" + MSG_SHOULD_EQUAL);
  Assert.equal(patch.hashFunction, "SHA1",
               "the update patch hashFunction attribute" + MSG_SHOULD_EQUAL);
  Assert.equal(patch.hashValue, "98db9dad8e1d80eda7e1170d0187d6f53e477059",
               "the update patch hashValue attribute" + MSG_SHOULD_EQUAL);
  Assert.equal(patch.size, 9856459,
               "the update patch size attribute" + MSG_SHOULD_EQUAL);
  
  
  
  
  Assert.equal(typeof(patch.state), "string",
               "the update patch state typeof value should equal |string|");
  Assert.equal(patch.state, STATE_NONE,
               "the update patch state attribute" + MSG_SHOULD_EQUAL);
  Assert.ok(!patch.selected,
            "the update patch selected attribute" + MSG_SHOULD_EQUAL);
  

  patch = bestUpdate.getPatchAt(1);
  Assert.equal(patch.type, "partial",
               "the update patch type attribute" + MSG_SHOULD_EQUAL);
  Assert.equal(patch.URL, "http://partial/",
               "the update patch URL attribute" + MSG_SHOULD_EQUAL);
  Assert.equal(patch.hashFunction, "SHA1",
               "the update patch hashFunction attribute" + MSG_SHOULD_EQUAL);
  Assert.equal(patch.hashValue, "e6678ca40ae7582316acdeddf3c133c9c8577de4",
               "the update patch hashValue attribute" + MSG_SHOULD_EQUAL);
  Assert.equal(patch.size, 1316138,
               "the update patch size attribute" + MSG_SHOULD_EQUAL);
  Assert.equal(patch.state, STATE_NONE,
               "the update patch state attribute" + MSG_SHOULD_EQUAL);
  Assert.ok(!patch.selected,
            "the update patch selected attribute" + MSG_SHOULD_EQUAL);
  

  run_test_pt03();
}


function run_test_pt03() {
  debugDump("testing one update available and the update's property values " +
            "with the format prior to bug 530872");
  gUpdates = null;
  gUpdateCount = null;
  gCheckFunc = check_test_pt03;
  let patches = getRemotePatchString("complete", "http://complete/", "SHA1",
                                     "98db9dad8e1d80eda7e1170d0187d6f53e477059",
                                     "9856459");
  let updates = getRemoteUpdateString(patches, "major", "Major Test",
                                      null, null,
                                      "5.1a1pre", "20080811053724",
                                      "http://details/",
                                      null, null, null, null, "691200",
                                      null, "version 4.1a1pre", "4.1a1pre");
  gResponseBody = getRemoteUpdatesXMLString(updates);
  gUpdateChecker.checkForUpdates(updateCheckListener, true);
}

function check_test_pt03() {
  Assert.equal(gUpdateCount, 1,
               "the update count" + MSG_SHOULD_EQUAL);
  let bestUpdate = gAUS.selectUpdate(gUpdates, gUpdateCount);
  Assert.equal(bestUpdate.type, "major",
               "the update type attribute" + MSG_SHOULD_EQUAL);
  Assert.equal(bestUpdate.name, "Major Test",
               "the update name attribute" + MSG_SHOULD_EQUAL);
  Assert.equal(bestUpdate.displayVersion, "version 4.1a1pre",
               "the update displayVersion attribute" + MSG_SHOULD_EQUAL);
  Assert.equal(bestUpdate.appVersion, "4.1a1pre",
               "the update appVersion attribute" + MSG_SHOULD_EQUAL);
  Assert.equal(bestUpdate.platformVersion, "5.1a1pre",
               "the update platformVersion attribute" + MSG_SHOULD_EQUAL);
  Assert.equal(bestUpdate.buildID, "20080811053724",
               "the update buildID attribute" + MSG_SHOULD_EQUAL);
  Assert.equal(bestUpdate.detailsURL, "http://details/",
               "the update detailsURL attribute" + MSG_SHOULD_EQUAL);
  Assert.equal(bestUpdate.billboardURL, "http://details/",
               "the update billboardURL attribute" + MSG_SHOULD_EQUAL);
  Assert.ok(!bestUpdate.licenseURL,
            "the update licenseURL attribute" + MSG_SHOULD_EQUAL);
  Assert.ok(bestUpdate.showPrompt,
            "the update showPrompt attribute" + MSG_SHOULD_EQUAL);
  Assert.ok(bestUpdate.showNeverForVersion,
            "the update showNeverForVersion attribute" + MSG_SHOULD_EQUAL);
  Assert.equal(bestUpdate.promptWaitTime, "691200",
               "the update promptWaitTime attribute" + MSG_SHOULD_EQUAL);
  Assert.equal(bestUpdate.serviceURL, URL_HOST + "/update.xml?force=1",
               "the update serviceURL attribute" + MSG_SHOULD_EQUAL);
  Assert.equal(bestUpdate.channel, "test_channel",
               "the update channel attribute" + MSG_SHOULD_EQUAL);
  Assert.ok(!bestUpdate.isCompleteUpdate,
            "the update isCompleteUpdate attribute" + MSG_SHOULD_EQUAL);
  Assert.ok(!bestUpdate.isSecurityUpdate,
            "the update isSecurityUpdate attribute" + MSG_SHOULD_EQUAL);
  
  Assert.ok((Date.now() - bestUpdate.installDate) < 10000,
            "the update installDate attribute should be within 10 seconds of " +
            "the current time");
  Assert.ok(!bestUpdate.statusText,
            "the update statusText attribute" + MSG_SHOULD_EQUAL);
  
  
  Assert.equal(bestUpdate.state, "",
               "the update state attribute" + MSG_SHOULD_EQUAL);
  Assert.equal(bestUpdate.errorCode, 0,
               "the update errorCode attribute" + MSG_SHOULD_EQUAL);
  Assert.equal(bestUpdate.patchCount, 1,
               "the update patchCount attribute" + MSG_SHOULD_EQUAL);
  

  let patch = bestUpdate.getPatchAt(0);
  Assert.equal(patch.type, "complete",
               "the update patch type attribute" + MSG_SHOULD_EQUAL);
  Assert.equal(patch.URL, "http://complete/",
               "the update patch URL attribute" + MSG_SHOULD_EQUAL);
  Assert.equal(patch.hashFunction, "SHA1",
               "the update patch hashFunction attribute" + MSG_SHOULD_EQUAL);
  Assert.equal(patch.hashValue, "98db9dad8e1d80eda7e1170d0187d6f53e477059",
               "the update patch hashValue attribute" + MSG_SHOULD_EQUAL);
  Assert.equal(patch.size, 9856459,
               "the update patch size attribute" + MSG_SHOULD_EQUAL);
  
  
  
  
  Assert.equal(typeof(patch.state), "string",
               "the update patch state typeof value should equal |string|");
  Assert.equal(patch.state, STATE_NONE,
               "the update patch state attribute" + MSG_SHOULD_EQUAL);
  Assert.ok(!patch.selected,
            "the update patch selected attribute" + MSG_SHOULD_EQUAL);
  

  run_test_pt04();
}


function run_test_pt04() {
  gResponseBody = "<parsererror/>";
  run_test_helper_pt1("testing empty update xml",
                      null, run_test_pt05);
}


function run_test_pt05() {
  gResponseBody = getRemoteUpdatesXMLString("");
  run_test_helper_pt1("testing no updates available",
                      0, run_test_pt06);
}


function run_test_pt06() {
  let patches = getRemotePatchString("complete");
  patches += getRemotePatchString("partial");
  let updates = getRemoteUpdateString(patches);
  gResponseBody = getRemoteUpdatesXMLString(updates);
  run_test_helper_pt1("testing one update available",
                      1, run_test_pt07);
}


function run_test_pt07() {
  let patches = getRemotePatchString("complete");
  patches += getRemotePatchString("partial");
  let updates = getRemoteUpdateString(patches);
  updates += getRemoteUpdateString(patches);
  updates += getRemoteUpdateString(patches);
  gResponseBody = getRemoteUpdatesXMLString(updates);
  run_test_helper_pt1("testing three updates available",
                      3, run_test_pt08);
}



function run_test_pt08() {
  let patches = getRemotePatchString("complete", null, null, null, "0");
  patches += getRemotePatchString("partial", null, null, null, "0");
  let updates = getRemoteUpdateString(patches);
  gResponseBody = getRemoteUpdatesXMLString(updates);
  run_test_helper_pt1("testing one update with complete and partial " +
                      "patches with size 0", 0, run_test_pt09);
}


function run_test_pt09() {
  let patches = getRemotePatchString("complete", null, null, null, "0");
  let updates = getRemoteUpdateString(patches);
  gResponseBody = getRemoteUpdatesXMLString(updates);
  run_test_helper_pt1("testing one update with complete patch with size 0",
                      0, run_test_pt10);
}


function run_test_pt10() {
  let patches = getRemotePatchString("partial", null, null, null, "0");
  let updates = getRemoteUpdateString(patches);
  gResponseBody = getRemoteUpdatesXMLString(updates);
  run_test_helper_pt1("testing one update with partial patch with size 0",
                      0, run_test_pt11);
}


function run_test_pt11() {
  let patches = getRemotePatchString("complete");
  patches += getRemotePatchString("partial");
  let updates = getRemoteUpdateString(patches, "minor", null, null, "1.0pre");
  updates += getRemoteUpdateString(patches, "minor", null, null, "1.0a");
  gResponseBody = getRemoteUpdatesXMLString(updates);
  run_test_helper_pt1("testing two updates older than the current version",
                      2, check_test_pt11);
}

function check_test_pt11() {
  let bestUpdate = gAUS.selectUpdate(gUpdates, gUpdateCount);
  Assert.ok(!bestUpdate);
  run_test_pt12();
}


function run_test_pt12() {
  let patches = getRemotePatchString("complete");
  patches += getRemotePatchString("partial");
  let updates = getRemoteUpdateString(patches, "minor", null, "version 1.0");
  gResponseBody = getRemoteUpdatesXMLString(updates);
  run_test_helper_pt1("testing one update equal to the current version",
                      1, check_test_pt12);
}

function check_test_pt12() {
  let bestUpdate = gAUS.selectUpdate(gUpdates, gUpdateCount);
  Assert.ok(!!bestUpdate);
  Assert.equal(bestUpdate.displayVersion, "version 1.0",
               "the update displayVersion attribute" + MSG_SHOULD_EQUAL);

  doTestFinish();
}
