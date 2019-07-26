


"use strict";

function test() {
  waitForExplicitFinish();
  resetPreferences();
  registerCleanupFunction(resetPreferences);

  function observer(win, topic, data) {
    Services.obs.removeObserver(observer, "advanced-pane-loaded");
    runTest(win);
  }
  Services.obs.addObserver(observer, "advanced-pane-loaded", false);
  openDialog("chrome://browser/content/preferences/preferences.xul", "Preferences",
             "chrome,titlebar,toolbar,centerscreen,dialog=no", "paneAdvanced");
}

function runTest(win) {
  let doc = win.document;

  
  
  
  

  let checkbox = doc.getElementById("submitHealthReportBox");
  ok(checkbox);
  is(checkbox.checked, false, "Health Report checkbox is unchecked on app first run.");

  let reporter = Components.classes["@mozilla.org/healthreport/service;1"]
                                   .getService(Components.interfaces.nsISupports)
                                   .wrappedJSObject
                                   .reporter;
  ok(reporter);
  is(reporter.dataSubmissionPolicyAccepted, false, "Data submission policy not accepted.");

  checkbox.checked = true;
  checkbox.doCommand();
  is(reporter.dataSubmissionPolicyAccepted, true, "Checking checkbox accepts data submission policy.");
  checkbox.checked = false;
  checkbox.doCommand();
  is(reporter.dataSubmissionPolicyAccepted, false, "Unchecking checkbox opts out of data submission.");

  win.close();
  finish();
}

function resetPreferences() {
  Services.prefs.clearUserPref("healthreport.policy.dataSubmissionPolicyAccepted");
}
