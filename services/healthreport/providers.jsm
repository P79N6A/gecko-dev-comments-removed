













"use strict";

this.EXPORTED_SYMBOLS = [
  "AppInfoProvider",
  "SessionsProvider",
  "SysInfoProvider",
];

const {classes: Cc, interfaces: Ci, utils: Cu} = Components;

Cu.import("resource://gre/modules/Metrics.jsm");
Cu.import("resource://gre/modules/Services.jsm");
Cu.import("resource://gre/modules/Task.jsm");
Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://services-common/preferences.js");
Cu.import("resource://services-common/utils.js");


XPCOMUtils.defineLazyModuleGetter(this, "UpdateChannel",
                                  "resource://gre/modules/UpdateChannel.jsm");







function AppInfoMeasurement() {
  Metrics.Measurement.call(this);
}

AppInfoMeasurement.prototype = Object.freeze({
  __proto__: Metrics.Measurement.prototype,

  name: "appinfo",
  version: 1,

  LAST_TEXT_FIELDS: [
    "vendor",
    "name",
    "id",
    "version",
    "appBuildID",
    "platformVersion",
    "platformBuildID",
    "os",
    "xpcomabi",
    "updateChannel",
    "distributionID",
    "distributionVersion",
    "hotfixVersion",
    "locale",
  ],

  configureStorage: function () {
    let self = this;
    return Task.spawn(function configureStorage() {
      for (let field of self.LAST_TEXT_FIELDS) {
        yield self.registerStorageField(field, self.storage.FIELD_LAST_TEXT);
      }
    });
  },
});


function AppVersionMeasurement() {
  Metrics.Measurement.call(this);
}

AppVersionMeasurement.prototype = Object.freeze({
  __proto__: Metrics.Measurement.prototype,

  name: "versions",
  version: 1,

  configureStorage: function () {
    return this.registerStorageField("version",
                                     this.storage.FIELD_DAILY_DISCRETE_TEXT);
  },
});



this.AppInfoProvider = function AppInfoProvider() {
  Metrics.Provider.call(this);

  this._prefs = new Preferences({defaultBranch: null});
}
AppInfoProvider.prototype = Object.freeze({
  __proto__: Metrics.Provider.prototype,

  name: "org.mozilla.appInfo",

  measurementTypes: [AppInfoMeasurement, AppVersionMeasurement],

  appInfoFields: {
    
    vendor: "vendor",
    name: "name",
    id: "ID",
    version: "version",
    appBuildID: "appBuildID",
    platformVersion: "platformVersion",
    platformBuildID: "platformBuildID",

    
    os: "OS",
    xpcomabi: "XPCOMABI",
  },

  onInit: function () {
    return Task.spawn(this._onInit.bind(this));
  },

  _onInit: function () {
    
    
    let ai;
    try {
      ai = Services.appinfo;
    } catch (ex) {
      this._log.error("Could not obtain Services.appinfo: " +
                     CommonUtils.exceptionStr(ex));
      yield this._setCurrentVersion("");
      return;
    }

    if (!ai) {
      this._log.error("Services.appinfo is unavailable.");
      yield this._setCurrentVersion("");
      return;
    }

    let currentVersion = ai.version;
    let lastVersion = yield this.getState("lastVersion");

    if (currentVersion == lastVersion) {
      return;
    }

    yield this._setCurrentVersion(currentVersion);
  },

  _setCurrentVersion: function (version) {
    this._log.info("Recording new application version: " + version);
    let m = this.getMeasurement("versions", 1);
    m.addDailyDiscreteText("version", version);
    return this.setState("lastVersion", version);
  },

  collectConstantData: function () {
    return this.enqueueStorageOperation(function collect() {
      return Task.spawn(this._populateConstants.bind(this));
    }.bind(this));
  },

  _populateConstants: function () {
    let m = this.getMeasurement(AppInfoMeasurement.prototype.name,
                                AppInfoMeasurement.prototype.version);

    let ai;
    try {
      ai = Services.appinfo;
    } catch (ex) {
      this._log.warn("Could not obtain Services.appinfo: " +
                     CommonUtils.exceptionStr(ex));
      throw ex;
    }

    if (!ai) {
      this._log.warn("Services.appinfo is unavailable.");
      throw ex;
    }

    for (let [k, v] in Iterator(this.appInfoFields)) {
      try {
        yield m.setLastText(k, ai[v]);
      } catch (ex) {
        this._log.warn("Error obtaining Services.appinfo." + v);
      }
    }

    try {
      yield m.setLastText("updateChannel", UpdateChannel.get());
    } catch (ex) {
      this._log.warn("Could not obtain update channel: " +
                     CommonUtils.exceptionStr(ex));
    }

    yield m.setLastText("distributionID", this._prefs.get("distribution.id", ""));
    yield m.setLastText("distributionVersion", this._prefs.get("distribution.version", ""));
    yield m.setLastText("hotfixVersion", this._prefs.get("extensions.hotfix.lastVersion", ""));

    try {
      let locale = Cc["@mozilla.org/chrome/chrome-registry;1"]
                     .getService(Ci.nsIXULChromeRegistry)
                     .getSelectedLocale("global");
      yield m.setLastText("locale", locale);
    } catch (ex) {
      this._log.warn("Could not obtain application locale: " +
                     CommonUtils.exceptionStr(ex));
    }
  },
});


function SysInfoMeasurement() {
  Metrics.Measurement.call(this);
}

SysInfoMeasurement.prototype = Object.freeze({
  __proto__: Metrics.Measurement.prototype,

  name: "sysinfo",
  version: 1,

  configureStorage: function () {
    return Task.spawn(function configureStorage() {
      yield this.registerStorageField("cpuCount", this.storage.FIELD_LAST_NUMERIC);
      yield this.registerStorageField("memoryMB", this.storage.FIELD_LAST_NUMERIC);
      yield this.registerStorageField("manufacturer", this.storage.FIELD_LAST_TEXT);
      yield this.registerStorageField("device", this.storage.FIELD_LAST_TEXT);
      yield this.registerStorageField("hardware", this.storage.FIELD_LAST_TEXT);
      yield this.registerStorageField("name", this.storage.FIELD_LAST_TEXT);
      yield this.registerStorageField("version", this.storage.FIELD_LAST_TEXT);
      yield this.registerStorageField("architecture", this.storage.FIELD_LAST_TEXT);
    }.bind(this));
  },
});


this.SysInfoProvider = function SysInfoProvider() {
  Metrics.Provider.call(this);
};

SysInfoProvider.prototype = Object.freeze({
  __proto__: Metrics.Provider.prototype,

  name: "org.mozilla.sysinfo",

  measurementTypes: [SysInfoMeasurement],

  sysInfoFields: {
    cpucount: "cpuCount",
    memsize: "memoryMB",
    manufacturer: "manufacturer",
    device: "device",
    hardware: "hardware",
    name: "name",
    version: "version",
    arch: "architecture",
  },

  collectConstantData: function () {
    return this.enqueueStorageOperation(function collection() {
      return Task.spawn(this._populateConstants.bind(this));
    }.bind(this));
  },

  _populateConstants: function () {
    let m = this.getMeasurement(SysInfoMeasurement.prototype.name,
                                SysInfoMeasurement.prototype.version);

    let si = Cc["@mozilla.org/system-info;1"]
               .getService(Ci.nsIPropertyBag2);

    for (let [k, v] in Iterator(this.sysInfoFields)) {
      try {
        if (!si.hasKey(k)) {
          this._log.debug("Property not available: " + k);
          continue;
        }

        let value = si.getProperty(k);
        let method = "setLastText";

        if (["cpucount", "memsize"].indexOf(k) != -1) {
          let converted = parseInt(value, 10);
          if (Number.isNaN(converted)) {
            continue;
          }

          value = converted;
          method = "setLastNumeric";
        }

        
        if (k == "memsize") {
          value = Math.round(value / 1048576);
        }

        yield m[method](v, value);
      } catch (ex) {
        this._log.warn("Error obtaining system info field: " + k + " " +
                       CommonUtils.exceptionStr(ex));
      }
    }
  },
});








function CurrentSessionMeasurement() {
  Metrics.Measurement.call(this);
}

CurrentSessionMeasurement.prototype = Object.freeze({
  __proto__: Metrics.Measurement.prototype,

  name: "current",
  version: 1,

  LAST_NUMERIC_FIELDS: [
    
    
    
    "startDay",

    
    "activeTime",

    
    "totalTime",

    
    "main",
    "firstPaint",
    "sessionRestored",
  ],

  configureStorage: function () {
    return Task.spawn(function configureStorage() {
      for (let field of this.LAST_NUMERIC_FIELDS) {
        yield this.registerStorageField(field, this.storage.FIELD_LAST_NUMERIC);
      }
    }.bind(this));
  },
});





function PreviousSessionsMeasurement() {
  Metrics.Measurement.call(this);
}

PreviousSessionsMeasurement.prototype = Object.freeze({
  __proto__: Metrics.Measurement.prototype,

  name: "previous",
  version: 1,

  DAILY_DISCRETE_NUMERIC_FIELDS: [
    
    "cleanActiveTime",
    "cleanTotalTime",

    
    "abortedActiveTime",
    "abortedTotalTime",

    
    "main",
    "firstPaint",
    "sessionRestored",
  ],

  configureStorage: function () {
    return Task.spawn(function configureStorage() {
      for (let field of this.DAILY_DISCRETE_NUMERIC_FIELDS) {
        yield this.registerStorageField(field, this.storage.FIELD_DAILY_DISCRETE_NUMERIC);
      }
    }.bind(this));
  },
});





















this.SessionsProvider = function () {
  Metrics.Provider.call(this);

  this._startDate = null;
  this._currentActiveTime = null;
  this._lastActivityDate = null;
  this._lastActivityWasInactive = false;
};

SessionsProvider.prototype = Object.freeze({
  __proto__: Metrics.Provider.prototype,

  name: "org.mozilla.appSessions",

  measurementTypes: [CurrentSessionMeasurement, PreviousSessionsMeasurement],

  _OBSERVERS: ["user-interaction-active", "user-interaction-inactive"],

  onInit: function () {
    return Task.spawn(this._onInit.bind(this));
  },

  _onInit: function () {
    
    
    let now = new Date();
    this._startDate = now;
    let current = this.getMeasurement("current", 1);

    
    
    let currentData = yield this.storage.getMeasurementLastValuesFromMeasurementID(current.id);

    
    
    if (currentData.size) {
      this._log.info("Data left over from old session. Counting as aborted.");
      yield Task.spawn(this._moveCurrentToDaily.bind(this, currentData, true));
    }

    this._currentActiveTime = 0;
    this._lastActivityDate = now;

    this._log.debug("Registering new/current session.");
    yield current.setLastNumeric("activeTime", 0, now);
    yield current.setLastNumeric("totalTime", 0, now);
    yield current.setLastNumeric("startDay", this._dateToDays(now), now);

    let si = this._getStartupInfo();

    for (let field of ["main", "firstPaint", "sessionRestored"]) {
      if (!(field in si)) {
        continue;
      }

      
      let value = si[field] - si.process;
      yield current.setLastNumeric(field, value, now);
    }

    for (let channel of this._OBSERVERS) {
      Services.obs.addObserver(this, channel, false);
    }
  },

  onShutdown: function () {
    for (let channel of this._OBSERVERS) {
      Services.obs.removeObserver(this, channel);
    }

    return Task.spawn(this._onShutdown.bind(this));
  },

  _onShutdown: function () {
    this._log.debug("Recording clean shutdown.");
    yield this.recordBrowserActivity(true);
    let current = this.getMeasurement("current", 1);

    let self = this;
    yield this.enqueueStorageOperation(function doShutdown() {
      return Task.spawn(function shutdownTask() {
        let data = yield self.storage.getMeasurementLastValuesFromMeasurementID(current.id);
        yield self._moveCurrentToDaily(data, false);
      });
    });
  },

  












  recordBrowserActivity: function (active) {
    
    
    
    let now = new Date();
    this._log.trace("Recording browser activity. Active? " + !!active);

    let m = this.getMeasurement("current", 1);

    let updateActive = active && !this._lastActivityWasInactive;
    this._lastActivityWasInactive = !active;

    if (updateActive) {
      this._currentActiveTime += now - this._lastActivityDate;
    }

    this._lastActivityDate = now;

    let totalTime = now - this._startDate;
    let activeTime = this._currentActiveTime;

    return this.enqueueStorageOperation(function op() {
      let promise = m.setLastNumeric("totalTime", totalTime, now);

      if (!updateActive) {
        return promise;
      }

      return m.setLastNumeric("activeTime", activeTime, now);
    });
  },

  _moveCurrentToDaily: function (fields, aborted) {
    this._log.debug("Moving current session to past. Aborted? " + aborted);
    let current = this.getMeasurement("current", 1);

    function clearCurrent() {
      current.deleteLastNumeric("startDay");
      current.deleteLastNumeric("activeTime");
      current.deleteLastNumeric("totalTime");
      current.deleteLastNumeric("main");
      current.deleteLastNumeric("firstPaint");
      return current.deleteLastNumeric("sessionRestored");
    }

    
    
    if (!fields.has("startDay") || !fields.has("activeTime") || !fields.has("totalTime")) {
      yield clearCurrent();
      return;
    }

    let daily = this.getMeasurement("previous", 1);

    let startDays = fields.get("startDay")[1];
    let activeTime = fields.get("activeTime")[1];
    let totalTime = fields.get("totalTime")[1];

    let date = this._daysToDate(startDays);
    let type = aborted ? "aborted" : "clean";

    yield daily.addDailyDiscreteNumeric(type + "ActiveTime", activeTime, date);
    yield daily.addDailyDiscreteNumeric(type + "TotalTime", totalTime, date);

    for (let field of ["main", "firstPaint", "sessionRestored"]) {
      if (!fields.has(field)) {
        this._log.info(field + " field not recorded for current session.");
        continue;
      }

      yield daily.addDailyDiscreteNumeric(field, fields.get(field)[1], date);
    }

    yield clearCurrent();
  },

  observe: function (subject, topic, data) {
    switch (topic) {
      case "user-interaction-active":
        this.recordBrowserActivity(true);
        break;

      case "user-interaction-inactive":
        this.recordBrowserActivity(false);
        break;
    }
  },

  
  _getStartupInfo: function () {
    return Cc["@mozilla.org/toolkit/app-startup;1"]
             .getService(Ci.nsIAppStartup)
             .getStartupInfo();
  },
});

