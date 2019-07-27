



"use strict";

const {classes: Cc, interfaces: Ci, utils: Cu} = Components;

Cu.import("resource://gre/modules/Preferences.jsm");
Cu.import("resource://gre/modules/Services.jsm");

const prefs = new Preferences("datareporting.healthreport.");

let healthReportWrapper = {
  init: function () {
    let iframe = document.getElementById("remote-report");
    iframe.addEventListener("load", healthReportWrapper.initRemotePage, false);
    iframe.src = this._getReportURI().spec;
    iframe.onload = () => {
      MozSelfSupport.getHealthReportPayload().then(this.updatePayload,
                                                   this.handleInitFailure);
    };
    prefs.observe("uploadEnabled", this.updatePrefState, healthReportWrapper);
  },

  uninit: function () {
    prefs.ignore("uploadEnabled", this.updatePrefState, healthReportWrapper);
  },

  _getReportURI: function () {
    let url = Services.urlFormatter.formatURLPref("datareporting.healthreport.about.reportUrl");
    return Services.io.newURI(url, null, null);
  },

  setDataSubmission: function (enabled) {
    MozSelfSupport.healthReportDataSubmissionEnabled = enabled;
    this.updatePrefState();
  },

  updatePrefState: function () {
    try {
      let prefs = {
        enabled: MozSelfSupport.healthReportDataSubmissionEnabled,
      };
      healthReportWrapper.injectData("prefs", prefs);
    }
    catch (ex) {
      healthReportWrapper.reportFailure(healthReportWrapper.ERROR_PREFS_FAILED);
    }
  },

  sendTelemetryPingList: function () {
    console.log("AboutHealthReport: Collecting Telemetry ping list.");
    MozSelfSupport.getTelemetryPingList().then((list) => {
      console.log("AboutHealthReport: Sending Telemetry ping list.");
      this.injectData("telemetry-ping-list", list);
    }).catch((ex) => {
      console.log("AboutHealthReport: Collecting ping list failed: " + ex);
    });
  },

  sendTelemetryPingData: function (pingId) {
    console.log("AboutHealthReport: Collecting Telemetry ping data.");
    MozSelfSupport.getTelemetryPing(pingId).then((ping) => {
      console.log("AboutHealthReport: Sending Telemetry ping data.");
      this.injectData("telemetry-ping-data", {
        id: pingId,
        pingData: ping,
      });
    }).catch((ex) => {
      console.log("AboutHealthReport: Loading ping data failed: " + ex);
      this.injectData("telemetry-ping-data", {
        id: pingId,
        error: "error-generic",
      });
    });
  },

  sendCurrentEnvironment: function () {
    console.log("AboutHealthReport: Sending Telemetry environment data.");
    MozSelfSupport.getCurrentTelemetryEnvironment().then((environment) => {
      this.injectData("telemetry-current-environment-data", environment);
    }).catch((ex) => {
      console.log("AboutHealthReport: Collecting current environment data failed: " + ex);
    });
  },

  sendCurrentPingData: function () {
    console.log("AboutHealthReport: Sending current Telemetry ping data.");
    MozSelfSupport.getCurrentTelemetrySubsessionPing().then((ping) => {
      this.injectData("telemetry-current-ping-data", ping);
    }).catch((ex) => {
      console.log("AboutHealthReport: Collecting current ping data failed: " + ex);
    });
  },

  refreshPayload: function () {
    MozSelfSupport.getHealthReportPayload().then(this.updatePayload,
                                                 this.handlePayloadFailure);
  },

  updatePayload: function (payload) {
    healthReportWrapper.injectData("payload", JSON.stringify(payload));
  },

  injectData: function (type, content) {
    let report = this._getReportURI();

    
    
    let reportUrl = report.scheme == "file" ? "*" : report.spec;

    let data = {
      type: type,
      content: content
    }

    let iframe = document.getElementById("remote-report");
    iframe.contentWindow.postMessage(data, reportUrl);
  },

  handleRemoteCommand: function (evt) {
    switch (evt.detail.command) {
      case "DisableDataSubmission":
        this.setDataSubmission(false);
        break;
      case "EnableDataSubmission":
        this.setDataSubmission(true);
        break;
      case "RequestCurrentPrefs":
        this.updatePrefState();
        break;
      case "RequestCurrentPayload":
        this.refreshPayload();
        break;
      case "RequestTelemetryPingList":
        this.sendTelemetryPingList();
        break;
      case "RequestTelemetryPingData":
        this.sendTelemetryPingData(evt.detail.id);
        break;
      case "RequestCurrentEnvironment":
        this.sendCurrentEnvironment();
        break;
      case "RequestCurrentPingData":
        this.sendCurrentPingData();
        break;
      default:
        Cu.reportError("Unexpected remote command received: " + evt.detail.command + ". Ignoring command.");
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
    }
    healthReportWrapper.injectData("error", details);
  },

  handleInitFailure: function () {
    healthReportWrapper.reportFailure(healthReportWrapper.ERROR_INIT_FAILED);
  },

  handlePayloadFailure: function () {
    healthReportWrapper.reportFailure(healthReportWrapper.ERROR_PAYLOAD_FAILED);
  },
}

window.addEventListener("load", function () { healthReportWrapper.init(); });
window.addEventListener("unload", function () { healthReportWrapper.uninit(); });
