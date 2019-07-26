#filter substitution





"use strict";

const { classes: Cc, interfaces: Ci, utils: Cu } = Components;

Cu.import("resource://gre/modules/commonjs/sdk/core/promise.js");
Cu.import("resource://gre/modules/OrderedBroadcast.jsm");
Cu.import("resource://gre/modules/Services.jsm");
Cu.import("resource://gre/modules/SharedPreferences.jsm");



const PREF_UPLOAD_ENABLED = "android.not_a_preference.healthreport.uploadEnabled";


const BROADCAST_ACTION_HEALTH_REPORT = "@ANDROID_PACKAGE_NAME@" + ".healthreport.request";


const PREF_REPORTURL = "datareporting.healthreport.about.reportUrl";

function sendMessageToJava(message) {
  return Cc["@mozilla.org/android/bridge;1"]
    .getService(Ci.nsIAndroidBridge)
    .handleGeckoMessage(JSON.stringify(message));
}


let sharedPrefs = new SharedPreferences();

let reporter = {
  onInit: function () {
    let deferred = Promise.defer();
    deferred.resolve();

    return deferred.promise;
  },

  collectAndObtainJSONPayload: function () {
    let deferred = Promise.defer();

    let callback = function (data, token, action) {
      if (data) {
        
        
        
        
        
        
        
        deferred.resolve(JSON.stringify(data));
      } else {
        deferred.reject();
      }
    };

    sendOrderedBroadcast(BROADCAST_ACTION_HEALTH_REPORT, null, callback);

    return deferred.promise;
  },
};

let policy = {
  get healthReportUploadEnabled() {
    return sharedPrefs.getBoolPref(PREF_UPLOAD_ENABLED);
  },

  recordHealthReportUploadEnabled: function (enabled) {
    sharedPrefs.setBoolPref(PREF_UPLOAD_ENABLED, !!enabled);
  },
};

let healthReportWrapper = {
  init: function () {
    reporter.onInit().then(healthReportWrapper.refreshPayload,
                           healthReportWrapper.handleInitFailure);

    let iframe = document.getElementById("remote-report");
    iframe.addEventListener("load", healthReportWrapper.initRemotePage, false);
    let report = this._getReportURI();
    iframe.src = report.spec;

    sharedPrefs.addObserver(PREF_UPLOAD_ENABLED, this, false);
  },

  observe: function (subject, topic, data) {
    if (topic != PREF_UPLOAD_ENABLED) {
      return;
    }

    subject.updatePrefState();
  },

  uninit: function () {
    sharedPrefs.removeObserver(PREF_UPLOAD_ENABLED, this);
  },

  _getReportURI: function () {
    let url = Services.urlFormatter.formatURLPref(PREF_REPORTURL);
    return Services.io.newURI(url, null, null);
  },

  onOptIn: function () {
    policy.recordHealthReportUploadEnabled(true,
                                           "Health report page sent opt-in command.");
    this.updatePrefState();
  },

  onOptOut: function () {
    policy.recordHealthReportUploadEnabled(false,
                                           "Health report page sent opt-out command.");
    this.updatePrefState();
  },

  updatePrefState: function () {
    try {
      let prefs = {
        enabled: policy.healthReportUploadEnabled,
      };
      this.injectData("prefs", prefs);
    } catch (e) {
      this.reportFailure(this.ERROR_PREFS_FAILED);
    }
  },

  refreshPayload: function () {
    reporter.collectAndObtainJSONPayload().then(healthReportWrapper.updatePayload,
                                                healthReportWrapper.handlePayloadFailure);
  },

  updatePayload: function (data) {
    healthReportWrapper.injectData("payload", data);
  },

  injectData: function (type, content) {
    let report = this._getReportURI();

    
    

    let reportUrl = report.scheme == "file" ? "*" : report.spec;

    let data = {
      type: type,
      content: content,
    };

    let iframe = document.getElementById("remote-report");
    iframe.contentWindow.postMessage(data, reportUrl);
  },

  handleRemoteCommand: function (evt) {
    switch (evt.detail.command) {
      case "DisableDataSubmission":
        this.onOptOut();
        break;
      case "EnableDataSubmission":
        this.onOptIn();
        break;
      case "RequestCurrentPrefs":
        this.updatePrefState();
        break;
      case "RequestCurrentPayload":
        this.refreshPayload();
        break;
      default:
        Cu.reportError("Unexpected remote command received: " + evt.detail.command +
                       ". Ignoring command.");
        break;
    }
  },

  initRemotePage: function () {
    let iframe = document.getElementById("remote-report").contentDocument;
    iframe.addEventListener("RemoteHealthReportCommand",
                            function onCommand(e) {healthReportWrapper.handleRemoteCommand(e);},
                            false);
    healthReportWrapper.updatePrefState();
  },

  
  ERROR_INIT_FAILED:    1,
  ERROR_PAYLOAD_FAILED: 2,
  ERROR_PREFS_FAILED:   3,

  reportFailure: function (error) {
    let details = {
      errorType: error,
    };
    healthReportWrapper.injectData("error", details);
  },

  handleInitFailure: function () {
    healthReportWrapper.reportFailure(healthReportWrapper.ERROR_INIT_FAILED);
  },

  handlePayloadFailure: function () {
    healthReportWrapper.reportFailure(healthReportWrapper.ERROR_PAYLOAD_FAILED);
  },
};
