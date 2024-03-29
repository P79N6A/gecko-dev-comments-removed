




"use strict";

const { classes: Cc, interfaces: Ci, utils: Cu } = Components;

Cu.import("resource://gre/modules/Services.jsm");
Cu.import("resource://gre/modules/Messaging.jsm");
Cu.import("resource://gre/modules/SharedPreferences.jsm");



const PREF_UPLOAD_ENABLED = "android.not_a_preference.healthreport.uploadEnabled";


const PREF_REPORTURL = "datareporting.healthreport.about.reportUrl";


const WRAPPER_VERSION = 1;

const EVENT_HEALTH_REQUEST = "HealthReport:Request";
const EVENT_HEALTH_RESPONSE = "HealthReport:Response";



let sharedPrefs = SharedPreferences.forApp();

let healthReportWrapper = {
  init: function () {
    let iframe = document.getElementById("remote-report");
    iframe.addEventListener("load", healthReportWrapper.initRemotePage, false);
    let report = this._getReportURI();
    iframe.src = report.spec;
    console.log("AboutHealthReport: loading content from " + report.spec);

    sharedPrefs.addObserver(PREF_UPLOAD_ENABLED, this, false);
    Services.obs.addObserver(this, EVENT_HEALTH_RESPONSE, false);
  },

  observe: function (subject, topic, data) {
    if (topic == PREF_UPLOAD_ENABLED) {
      this.updatePrefState();
    } else if (topic == EVENT_HEALTH_RESPONSE) {
      this.updatePayload(data);
    }
  },

  uninit: function () {
    sharedPrefs.removeObserver(PREF_UPLOAD_ENABLED, this);
    Services.obs.removeObserver(this, EVENT_HEALTH_RESPONSE);
  },

  _getReportURI: function () {
    let url = Services.urlFormatter.formatURLPref(PREF_REPORTURL);
    
    let uri = Services.io.newURI(url, null, null).QueryInterface(Ci.nsIURL);
    uri.query += ((uri.query != "") ? "&v=" : "v=") + WRAPPER_VERSION;
    return uri;
  },

  onOptIn: function () {
    console.log("AboutHealthReport: page sent opt-in command.");
    sharedPrefs.setBoolPref(PREF_UPLOAD_ENABLED, true);
    this.updatePrefState();
  },

  onOptOut: function () {
    console.log("AboutHealthReport: page sent opt-out command.");
    sharedPrefs.setBoolPref(PREF_UPLOAD_ENABLED, false);
    this.updatePrefState();
  },

  updatePrefState: function () {
    console.log("AboutHealthReport: sending pref state to page.");
    try {
      let prefs = {
        enabled: sharedPrefs.getBoolPref(PREF_UPLOAD_ENABLED),
      };
      this.injectData("prefs", prefs);
    } catch (e) {
      this.reportFailure(this.ERROR_PREFS_FAILED);
    }
  },

  refreshPayload: function () {
    console.log("AboutHealthReport: page requested fresh payload.");
    Messaging.sendRequest({
      type: EVENT_HEALTH_REQUEST,
    });
  },

  updatePayload: function (data) {
    healthReportWrapper.injectData("payload", data);
    
    
    console.log("AboutHealthReport: sending payload to page " +
         "(" + typeof(data) + " of length " + data.length + ").");
  },

  injectData: function (type, content) {
    let report = this._getReportURI();

    
    
    
    let reportUrl = (report.scheme == "file") ? "*" : report.spec;

    let data = {
      type: type,
      content: content,
    };

    let iframe = document.getElementById("remote-report");
    iframe.contentWindow.postMessage(data, reportUrl);
  },

  showSettings: function () {
    console.log("AboutHealthReport: showing settings.");
    Messaging.sendRequest({
      type: "Settings:Show",
      resource: "preferences_vendor",
    });
  },

  launchUpdater: function () {
    console.log("AboutHealthReport: launching updater.");
    Messaging.sendRequest({
      type: "Updater:Launch",
    });
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
      case "ShowSettings":
        this.showSettings();
        break;
      case "LaunchUpdater":
        this.launchUpdater();
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
    healthReportWrapper.injectData("begin", null);
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

window.addEventListener("load", healthReportWrapper.init.bind(healthReportWrapper), false);
window.addEventListener("unload", healthReportWrapper.uninit.bind(healthReportWrapper), false);
