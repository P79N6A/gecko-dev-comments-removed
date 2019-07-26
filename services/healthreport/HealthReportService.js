



"use strict";

const {classes: Cc, interfaces: Ci, utils: Cu} = Components;

Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://services-common/preferences.js");


const BRANCH = "healthreport.";
const DEFAULT_LOAD_DELAY_MSEC = 10 * 1000;

































this.HealthReportService = function HealthReportService() {
  this.wrappedJSObject = this;

  this._prefs = new Preferences(BRANCH);

  this._reporter = null;
}

HealthReportService.prototype = {
  classID: Components.ID("{e354c59b-b252-4040-b6dd-b71864e3e35c}"),

  QueryInterface: XPCOMUtils.generateQI([Ci.nsIObserver,
                                         Ci.nsISupportsWeakReference]),

  observe: function observe(subject, topic, data) {
    
    if (!this._prefs.get("service.enabled", true)) {
      return;
    }

    let os = Cc["@mozilla.org/observer-service;1"]
               .getService(Ci.nsIObserverService);

    switch (topic) {
      case "app-startup":
        os.addObserver(this, "final-ui-startup", true);
        break;

      case "final-ui-startup":
        os.removeObserver(this, "final-ui-startup");

        let delayInterval = this._prefs.get("service.loadDelayMsec") ||
                            DEFAULT_LOAD_DELAY_MSEC;

        
        
        this.timer = Cc["@mozilla.org/timer;1"].createInstance(Ci.nsITimer);
        this.timer.initWithCallback({
          notify: function notify() {
            
            
            let reporter = this.reporter;
            delete this.timer;
          }.bind(this),
        }, delayInterval, this.timer.TYPE_ONE_SHOT);

        break;
    }
  },

  






  get reporter() {
    if (!this._prefs.get("service.enabled", true)) {
      return null;
    }

    if (this._reporter) {
      return this._reporter;
    }

    let ns = {};
    
    Cu.import("resource://gre/modules/Task.jsm", ns);
    Cu.import("resource://gre/modules/services/healthreport/healthreporter.jsm", ns);
    Cu.import("resource://services-common/log4moz.js", ns);

    
    
    const LOGGERS = [
      "Services.HealthReport",
      "Services.Metrics",
      "Services.BagheeraClient",
      "Sqlite.Connection.healthreport",
    ];

    let prefs = new Preferences(BRANCH + "logging.");
    if (prefs.get("consoleEnabled", true)) {
      let level = prefs.get("consoleLevel", "Warn");
      let appender = new ns.Log4Moz.ConsoleAppender();
      appender.level = ns.Log4Moz.Level[level] || ns.Log4Moz.Level.Warn;

      for (let name of LOGGERS) {
        let logger = ns.Log4Moz.repository.getLogger(name);
        logger.addAppender(appender);
      }
    }

    
    this._reporter = new ns.HealthReporter(BRANCH);

    return this._reporter;
  },
};

Object.freeze(HealthReportService.prototype);

this.NSGetFactory = XPCOMUtils.generateNSGetFactory([HealthReportService]);

