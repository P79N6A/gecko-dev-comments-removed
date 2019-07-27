



"use strict";

const {classes: Cc, interfaces: Ci, utils: Cu} = Components;

Cu.import("resource://gre/modules/Preferences.jsm");
Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://services-common/utils.js");
Cu.import("resource://gre/modules/Promise.jsm");
Cu.import("resource://gre/modules/Task.jsm");
Cu.import("resource://gre/modules/osfile.jsm");


const ROOT_BRANCH = "datareporting.";
const POLICY_BRANCH = ROOT_BRANCH + "policy.";
const SESSIONS_BRANCH = ROOT_BRANCH + "sessions.";
const HEALTHREPORT_BRANCH = ROOT_BRANCH + "healthreport.";
const HEALTHREPORT_LOGGING_BRANCH = HEALTHREPORT_BRANCH + "logging.";
const DEFAULT_LOAD_DELAY_MSEC = 10 * 1000;
const DEFAULT_LOAD_DELAY_FIRST_RUN_MSEC = 60 * 1000;




































this.DataReportingService = function () {
  this.wrappedJSObject = this;

  this._quitting = false;

  this._os = Cc["@mozilla.org/observer-service;1"]
               .getService(Ci.nsIObserverService);

  this._clientID = null;
  this._loadClientIdTask = null;
  this._saveClientIdTask = null;

  this._stateDir = null;
  this._stateFilePath = null;
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

        try {
          this._prefs = new Preferences(HEALTHREPORT_BRANCH);

          
          
          
          
          
          if (this._prefs.get("service.enabled", true)) {
            this.sessionRecorder = new SessionRecorder(SESSIONS_BRANCH);
            this.sessionRecorder.onStartup();
          }

          
          let policyPrefs = new Preferences(POLICY_BRANCH);
          this.policy = new DataReportingPolicy(policyPrefs, this._prefs, this);

          this._stateDir = OS.Path.join(OS.Constants.Path.profileDir, "datareporting");
          this._stateFilePath = OS.Path.join(this._stateDir, "state.json");

          this._os.addObserver(this, "sessionstore-windows-restored", true);
        } catch (ex) {
          Cu.reportError("Exception when initializing data reporting service: " +
                         CommonUtils.exceptionStr(ex));
        }
        break;

      case "sessionstore-windows-restored":
        this._os.removeObserver(this, "sessionstore-windows-restored");
        this._os.addObserver(this, "quit-application", false);

        let policy = this.policy;
        policy.startPolling();

        
        
        if (!this._prefs.get("service.enabled", true)) {
          return;
        }

        let haveFirstRun = this._prefs.get("service.firstRun", false);
        let delayInterval;

        if (haveFirstRun) {
          delayInterval = this._prefs.get("service.loadDelayMsec") ||
                          DEFAULT_LOAD_DELAY_MSEC;
        } else {
          delayInterval = this._prefs.get("service.loadDelayFirstRunMsec") ||
                          DEFAULT_LOAD_DELAY_FIRST_RUN_MSEC;
        }

        
        
        this.timer = Cc["@mozilla.org/timer;1"].createInstance(Ci.nsITimer);
        this.timer.initWithCallback({
          notify: function notify() {
            delete this.timer;

            
            
            if (this._quitting) {
              return;
            }

            
            
            
            
            
            let reporter = this.healthReporter;
            policy.ensureUserNotified();
          }.bind(this),
        }, delayInterval, this.timer.TYPE_ONE_SHOT);

        break;

      case "quit-application":
        this._os.removeObserver(this, "quit-application");
        this._quitting = true;

        
        
        
        
        if (this.timer) {
          this.timer.cancel();
        }

        if (this.policy) {
          this.policy.stopPolling();
        }
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
      Cu.reportError("Exception when obtaining health reporter: " +
                     CommonUtils.exceptionStr(ex));
    }

    return this._healthReporter;
  },

  _loadHealthReporter: function () {
    
    if (!this.policy) {
      throw new Error("this.policy not set.");
    }

    let ns = {};
    

    Cu.import("resource://gre/modules/Task.jsm", ns);
    Cu.import("resource://gre/modules/HealthReport.jsm", ns);
    Cu.import("resource://gre/modules/Log.jsm", ns);

    
    
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
      let appender = new ns.Log.ConsoleAppender();
      appender.level = ns.Log.Level[level] || ns.Log.Level.Warn;

      for (let name of LOGGERS) {
        let logger = ns.Log.repository.getLogger(name);
        logger.addAppender(appender);
      }
    }

    if (loggingPrefs.get("dumpEnabled", false)) {
      let level = loggingPrefs.get("dumpLevel", "Debug");
      let appender = new ns.Log.DumpAppender();
      appender.level = ns.Log.Level[level] || ns.Log.Level.Debug;

      for (let name of LOGGERS) {
        let logger = ns.Log.repository.getLogger(name);
        logger.addAppender(appender);
      }
    }

    this._healthReporter = new ns.HealthReporter(HEALTHREPORT_BRANCH,
                                                 this.policy,
                                                 this.sessionRecorder);

    
    
    this._healthReporter.init().then(function onInit() {
      this._prefs.set("service.firstRun", true);
    }.bind(this));
  },

  _loadClientID: function () {
    if (this._loadClientIdTask) {
      return this._loadClientIdTask;
    }

    this._loadClientIdTask = Task.spawn(function* () {
      
      
      
      
      

      
      try {
        let state = yield CommonUtils.readJSON(this._stateFilePath);
        if (state && 'clientID' in state && typeof(state.clientID) == 'string') {
          this._clientID = state.clientID;
          this._loadClientIdTask = null;
          return this._clientID;
        }
      } catch (e) {
        
      }

      
      try {
        let fhrStatePath = OS.Path.join(OS.Constants.Path.profileDir, "healthreport", "state.json");
        let state = yield CommonUtils.readJSON(fhrStatePath);
        if (state && 'clientID' in state && typeof(state.clientID) == 'string') {
          this._clientID = state.clientID;
          this._loadClientIdTask = null;
          this._saveClientID();
          return this._clientID;
        }
      } catch (e) {
        
      }

      
      this._clientID = CommonUtils.generateUUID();
      this._loadClientIdTask = null;
      this._saveClientIdTask = this._saveClientID();

      
      
      
      
      yield this._saveClientIdTask;

      return this._clientID;
    }.bind(this));

    return this._loadClientIdTask;
  },

  _saveClientID: Task.async(function* () {
    let obj = { clientID: this._clientID };
    yield OS.File.makeDir(this._stateDir);
    yield CommonUtils.writeJSON(obj, this._stateFilePath);
    this._saveClientIdTask = null;
  }),

  






  getClientID: function() {
    if (!this._clientID) {
      return this._loadClientID();
    }

    return Promise.resolve(this._clientID);
  },

  




  resetClientID: Task.async(function* () {
    yield this._loadClientIdTask;
    yield this._saveClientIdTask;

    this._clientID = CommonUtils.generateUUID();
    this._saveClientIdTask = this._saveClientID();
    yield this._saveClientIdTask;

    return this._clientID;
  }),

  


  _reset: Task.async(function* () {
    yield this._loadClientIdTask;
    yield this._saveClientIdTask;
    this._clientID = null;
  }),
});

this.NSGetFactory = XPCOMUtils.generateNSGetFactory([DataReportingService]);

#define MERGED_COMPARTMENT

#include ../common/observers.js
;
#include policy.jsm
;
#include sessions.jsm
;

