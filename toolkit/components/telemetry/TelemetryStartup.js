




"use strict";

const {classes: Cc, interfaces: Ci, utils: Cu} = Components;

Cu.import("resource://gre/modules/TelemetryController.jsm", this);
Cu.import("resource://gre/modules/TelemetrySession.jsm", this);
Cu.import("resource://gre/modules/TelemetryEnvironment.jsm", this);
Cu.import("resource://gre/modules/XPCOMUtils.jsm", this);





function TelemetryStartup() {
}

TelemetryStartup.prototype.classID = Components.ID("{117b219f-92fe-4bd2-a21b-95a342a9d474}");
TelemetryStartup.prototype.QueryInterface = XPCOMUtils.generateQI([Components.interfaces.nsIObserver]);
TelemetryStartup.prototype.observe = function(aSubject, aTopic, aData) {
  if (aTopic == "profile-after-change" || aTopic == "app-startup") {
    TelemetryController.observe(null, aTopic, null);
    TelemetrySession.observe(null, aTopic, null);
  }
  if (aTopic == "profile-after-change") {
    annotateEnvironment();
    TelemetryEnvironment.registerChangeListener("CrashAnnotator", annotateEnvironment);
    TelemetryEnvironment.onInitialized().then(() => annotateEnvironment());
  }
}

function annotateEnvironment() {
  try {
    let cr = Cc["@mozilla.org/toolkit/crash-reporter;1"]
      .getService(Ci.nsICrashReporter);
    let env = JSON.stringify(TelemetryEnvironment.currentEnvironment);
    cr.annotateCrashReport("TelemetryEnvironment", env);
  } catch (e) {
    
  }
}

this.NSGetFactory = XPCOMUtils.generateNSGetFactory([TelemetryStartup]);
