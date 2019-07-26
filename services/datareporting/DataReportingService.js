



"use strict";

const {classes: Cc, interfaces: Ci, utils: Cu} = Components;

Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://services-common/observers.js");
Cu.import("resource://services-common/preferences.js");
Cu.import("resource://services-common/utils.js");


const ROOT_BRANCH = "datareporting.";
const POLICY_BRANCH = ROOT_BRANCH + "policy.";
const SESSIONS_BRANCH = ROOT_BRANCH + "sessions.";
const HEALTHREPORT_BRANCH = ROOT_BRANCH + "healthreport.";
const HEALTHREPORT_LOGGING_BRANCH = HEALTHREPORT_BRANCH + "logging.";
const DEFAULT_LOAD_DELAY_MSEC = 10 * 1000;

































this.DataReportingService = function () {
  this.wrappedJSObject = this;

  this._quitting = false;

  this._os = Cc["@mozilla.org/observer-service;1"]
               .getService(Ci.nsIObserverService);
}

DataReportingService.prototype = Object.freeze({
  classID: Components.ID("{41f6ae36-a79f-4613-9ac3-915e70f83789}"),

  QueryInterface: XPCOMUtils.generateQI([Ci.nsIObserver,
                                         Ci.nsISupportsWeakReference]),

  
  
  

  


  onRequestDataUpload: function (request) {
    if (!this.healthReporter) {
      return;
    }

    this.healthReporter.requestDataUpload(request);
  },

  onNotifyDataPolicy: function (request) {
    Observers.notify("datareporting:notify-data-policy:request", request);
  },

  onRequestRemoteDelete: function (request) {
    if (!this.healthReporter) {
      return;
    }

    this.healthReporter.deleteRemoteData(request);
  },

  
  
  

  observe: function observe(subject, topic, data) {
    switch (topic) {
      case "app-startup":
        this._os.addObserver(this, "profile-after-change", true);
        break;

      case "profile-after-change":
        this._os.removeObserver(this, "profile-after-change");
        this._os.addObserver(this, "sessionstore-windows-restored", true);

        this._prefs = new Preferences(HEALTHREPORT_BRANCH);

        
        
        
        
        
        if (this._prefs.get("service.enabled", true)) {
          this.sessionRecorder = new SessionRecorder(SESSIONS_BRANCH);
          this.sessionRecorder.onStartup();
        }

        
        let policyPrefs = new Preferences(POLICY_BRANCH);
        this.policy = new DataReportingPolicy(policyPrefs, this._prefs, this);

        break;

      case "sessionstore-windows-restored":
        this._os.removeObserver(this, "sessionstore-windows-restored");
        this._os.addObserver(this, "quit-application", false);

        
        
        
        
        
        if (this.sessionRecorder) {
          CommonUtils.nextTick(this.sessionRecorder.recordStartupFields,
                               this.sessionRecorder);
        }

        this.policy.startPolling();

        
        
        if (!this._prefs.get("service.enabled", true)) {
          return;
        }

        let delayInterval = this._prefs.get("service.loadDelayMsec") ||
                            DEFAULT_LOAD_DELAY_MSEC;

        
        
        this.timer = Cc["@mozilla.org/timer;1"].createInstance(Ci.nsITimer);
        this.timer.initWithCallback({
          notify: function notify() {
            delete this.timer;

            
            
            if (this._quitting) {
              return;
            }

            
            
            
            
            
            let reporter = this.healthReporter;
          }.bind(this),
        }, delayInterval, this.timer.TYPE_ONE_SHOT);

        break;

      case "quit-application":
        this._os.removeObserver(this, "quit-application");
        this._quitting = true;

        
        
        
        
        if (this.timer) {
          this.timer.cancel();
        }

        this.policy.stopPolling();
        break;
    }
  },

  






  get healthReporter() {
    if (!this._prefs.get("service.enabled", true)) {
      return null;
    }

    if ("_healthReporter" in this) {
      return this._healthReporter;
    }

    try {
      this._loadHealthReporter();
    } catch (ex) {
      this._healthReporter = null;
    }

    return this._healthReporter;
  },

  _loadHealthReporter: function () {
    let ns = {};
    

    Cu.import("resource://gre/modules/Task.jsm", ns);
    Cu.import("resource://gre/modules/HealthReport.jsm", ns);
    Cu.import("resource://services-common/log4moz.js", ns);

    
    
    const LOGGERS = [
      "Services.DataReporting",
      "Services.HealthReport",
      "Services.Metrics",
      "Services.BagheeraClient",
      "Sqlite.Connection.healthreport",
    ];

    let loggingPrefs = new Preferences(HEALTHREPORT_LOGGING_BRANCH);
    if (loggingPrefs.get("consoleEnabled", true)) {
      let level = loggingPrefs.get("consoleLevel", "Warn");
      let appender = new ns.Log4Moz.ConsoleAppender();
      appender.level = ns.Log4Moz.Level[level] || ns.Log4Moz.Level.Warn;

      for (let name of LOGGERS) {
        let logger = ns.Log4Moz.repository.getLogger(name);
        logger.addAppender(appender);
      }
    }

    
    this._healthReporter = new ns.HealthReporter(HEALTHREPORT_BRANCH,
                                                 this.policy,
                                                 this.sessionRecorder);
  },
});

this.NSGetFactory = XPCOMUtils.generateNSGetFactory([DataReportingService]);

#define MERGED_COMPARTMENT

#include policy.jsm
;
#include sessions.jsm
;

