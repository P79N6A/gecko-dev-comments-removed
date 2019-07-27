













#ifndef MERGED_COMPARTMENT

"use strict";

this.EXPORTED_SYMBOLS = [
  "AddonsProvider",
  "AppInfoProvider",
#ifdef MOZ_CRASHREPORTER
  "CrashesProvider",
#endif
  "HealthReportProvider",
  "HotfixProvider",
  "PlacesProvider",
  "SearchesProvider",
  "SessionsProvider",
  "SysInfoProvider",
];

const {classes: Cc, interfaces: Ci, utils: Cu} = Components;

Cu.import("resource://gre/modules/Metrics.jsm");

#endif

Cu.import("resource://gre/modules/Promise.jsm");
Cu.import("resource://gre/modules/osfile.jsm");
Cu.import("resource://gre/modules/Preferences.jsm");
Cu.import("resource://gre/modules/Services.jsm");
Cu.import("resource://gre/modules/Task.jsm");
Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://services-common/utils.js");

XPCOMUtils.defineLazyModuleGetter(this, "AddonManager",
                                  "resource://gre/modules/AddonManager.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "UpdateChannel",
                                  "resource://gre/modules/UpdateChannel.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "PlacesDBUtils",
                                  "resource://gre/modules/PlacesDBUtils.jsm");


const LAST_NUMERIC_FIELD = {type: Metrics.Storage.FIELD_LAST_NUMERIC};
const LAST_TEXT_FIELD = {type: Metrics.Storage.FIELD_LAST_TEXT};
const DAILY_DISCRETE_NUMERIC_FIELD = {type: Metrics.Storage.FIELD_DAILY_DISCRETE_NUMERIC};
const DAILY_LAST_NUMERIC_FIELD = {type: Metrics.Storage.FIELD_DAILY_LAST_NUMERIC};
const DAILY_LAST_TEXT_FIELD = {type: Metrics.Storage.FIELD_DAILY_LAST_TEXT};
const DAILY_COUNTER_FIELD = {type: Metrics.Storage.FIELD_DAILY_COUNTER};

const TELEMETRY_PREF = "toolkit.telemetry.enabled";

function isTelemetryEnabled(prefs) {
  return prefs.get(TELEMETRY_PREF, false);
}







function AppInfoMeasurement() {
  Metrics.Measurement.call(this);
}

AppInfoMeasurement.prototype = Object.freeze({
  __proto__: Metrics.Measurement.prototype,

  name: "appinfo",
  version: 2,

  fields: {
    vendor: LAST_TEXT_FIELD,
    name: LAST_TEXT_FIELD,
    id: LAST_TEXT_FIELD,
    version: LAST_TEXT_FIELD,
    appBuildID: LAST_TEXT_FIELD,
    platformVersion: LAST_TEXT_FIELD,
    platformBuildID: LAST_TEXT_FIELD,
    os: LAST_TEXT_FIELD,
    xpcomabi: LAST_TEXT_FIELD,
    updateChannel: LAST_TEXT_FIELD,
    distributionID: LAST_TEXT_FIELD,
    distributionVersion: LAST_TEXT_FIELD,
    hotfixVersion: LAST_TEXT_FIELD,
    locale: LAST_TEXT_FIELD,
    isDefaultBrowser: {type: Metrics.Storage.FIELD_DAILY_LAST_NUMERIC},
    isTelemetryEnabled: {type: Metrics.Storage.FIELD_DAILY_LAST_NUMERIC},
    isBlocklistEnabled: {type: Metrics.Storage.FIELD_DAILY_LAST_NUMERIC},
  },
});







function AppInfoMeasurement1() {
  Metrics.Measurement.call(this);
}

AppInfoMeasurement1.prototype = Object.freeze({
  __proto__: Metrics.Measurement.prototype,

  name: "appinfo",
  version: 1,

  fields: {
    isDefaultBrowser: {type: Metrics.Storage.FIELD_DAILY_LAST_NUMERIC},
  },
});


function AppVersionMeasurement1() {
  Metrics.Measurement.call(this);
}

AppVersionMeasurement1.prototype = Object.freeze({
  __proto__: Metrics.Measurement.prototype,

  name: "versions",
  version: 1,

  fields: {
    version: {type: Metrics.Storage.FIELD_DAILY_DISCRETE_TEXT},
  },
});


function AppVersionMeasurement2() {
  Metrics.Measurement.call(this);
}

AppVersionMeasurement2.prototype = Object.freeze({
  __proto__: Metrics.Measurement.prototype,

  name: "versions",
  version: 2,

  fields: {
    appVersion: {type: Metrics.Storage.FIELD_DAILY_DISCRETE_TEXT},
    platformVersion: {type: Metrics.Storage.FIELD_DAILY_DISCRETE_TEXT},
    appBuildID: {type: Metrics.Storage.FIELD_DAILY_DISCRETE_TEXT},
    platformBuildID: {type: Metrics.Storage.FIELD_DAILY_DISCRETE_TEXT},
  },
});




function AppUpdateMeasurement1() {
  Metrics.Measurement.call(this);
}

AppUpdateMeasurement1.prototype = Object.freeze({
  __proto__: Metrics.Measurement.prototype,

  name: "update",
  version: 1,

  fields: {
    enabled: {type: Metrics.Storage.FIELD_DAILY_LAST_NUMERIC},
    autoDownload: {type: Metrics.Storage.FIELD_DAILY_LAST_NUMERIC},
  },
});

this.AppInfoProvider = function AppInfoProvider() {
  Metrics.Provider.call(this);

  this._prefs = new Preferences({defaultBranch: null});
}
AppInfoProvider.prototype = Object.freeze({
  __proto__: Metrics.Provider.prototype,

  name: "org.mozilla.appInfo",

  measurementTypes: [
    AppInfoMeasurement,
    AppInfoMeasurement1,
    AppUpdateMeasurement1,
    AppVersionMeasurement1,
    AppVersionMeasurement2,
  ],

  pullOnly: true,

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

  postInit: function () {
    return Task.spawn(this._postInit.bind(this));
  },

  _postInit: function () {
    let recordEmptyAppInfo = function () {
      this._setCurrentAppVersion("");
      this._setCurrentPlatformVersion("");
      this._setCurrentAppBuildID("");
      return this._setCurrentPlatformBuildID("");
    }.bind(this);

    
    
    let ai;
    try {
      ai = Services.appinfo;
    } catch (ex) {
      this._log.error("Could not obtain Services.appinfo: " +
                     CommonUtils.exceptionStr(ex));
      yield recordEmptyAppInfo();
      return;
    }

    if (!ai) {
      this._log.error("Services.appinfo is unavailable.");
      yield recordEmptyAppInfo();
      return;
    }

    let currentAppVersion = ai.version;
    let currentPlatformVersion = ai.platformVersion;
    let currentAppBuildID = ai.appBuildID;
    let currentPlatformBuildID = ai.platformBuildID;

    
    let lastAppVersion = yield this.getState("lastVersion");
    let lastPlatformVersion = yield this.getState("lastPlatformVersion");
    let lastAppBuildID = yield this.getState("lastAppBuildID");
    let lastPlatformBuildID = yield this.getState("lastPlatformBuildID");

    if (currentAppVersion != lastAppVersion) {
      yield this._setCurrentAppVersion(currentAppVersion);
    }

    if (currentPlatformVersion != lastPlatformVersion) {
      yield this._setCurrentPlatformVersion(currentPlatformVersion);
    }

    if (currentAppBuildID != lastAppBuildID) {
      yield this._setCurrentAppBuildID(currentAppBuildID);
    }

    if (currentPlatformBuildID != lastPlatformBuildID) {
      yield this._setCurrentPlatformBuildID(currentPlatformBuildID);
    }
  },

  _setCurrentAppVersion: function (version) {
    this._log.info("Recording new application version: " + version);
    let m = this.getMeasurement("versions", 2);
    m.addDailyDiscreteText("appVersion", version);

    
    return this.setState("lastVersion", version);
  },

  _setCurrentPlatformVersion: function (version) {
    this._log.info("Recording new platform version: " + version);
    let m = this.getMeasurement("versions", 2);
    m.addDailyDiscreteText("platformVersion", version);
    return this.setState("lastPlatformVersion", version);
  },

  _setCurrentAppBuildID: function (build) {
    this._log.info("Recording new application build ID: " + build);
    let m = this.getMeasurement("versions", 2);
    m.addDailyDiscreteText("appBuildID", build);
    return this.setState("lastAppBuildID", build);
  },

  _setCurrentPlatformBuildID: function (build) {
    this._log.info("Recording new platform build ID: " + build);
    let m = this.getMeasurement("versions", 2);
    m.addDailyDiscreteText("platformBuildID", build);
    return this.setState("lastPlatformBuildID", build);
  },


  collectConstantData: function () {
    return this.storage.enqueueTransaction(this._populateConstants.bind(this));
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

    
    yield this._recordIsTelemetryEnabled(m);
    yield this._recordIsBlocklistEnabled(m);
    yield this._recordDefaultBrowser(m);
  },

  _recordIsTelemetryEnabled: function (m) {
    let enabled = isTelemetryEnabled(this._prefs);
    this._log.debug("Recording telemetry enabled (" + TELEMETRY_PREF + "): " + enabled);
    yield m.setDailyLastNumeric("isTelemetryEnabled", enabled ? 1 : 0);
  },

  _recordIsBlocklistEnabled: function (m) {
    let enabled = this._prefs.get("extensions.blocklist.enabled", false);
    this._log.debug("Recording blocklist enabled: " + enabled);
    yield m.setDailyLastNumeric("isBlocklistEnabled", enabled ? 1 : 0);
  },

  _recordDefaultBrowser: function (m) {
    let shellService;
    try {
      shellService = Cc["@mozilla.org/browser/shell-service;1"]
                       .getService(Ci.nsIShellService);
    } catch (ex) {
      this._log.warn("Could not obtain shell service: " +
                     CommonUtils.exceptionStr(ex));
    }

    let isDefault = -1;

    if (shellService) {
      try {
        
        isDefault = shellService.isDefaultBrowser(false, true) ? 1 : 0;
      } catch (ex) {
        this._log.warn("Could not determine if default browser: " +
                       CommonUtils.exceptionStr(ex));
      }
    }

    return m.setDailyLastNumeric("isDefaultBrowser", isDefault);
  },

  collectDailyData: function () {
    return this.storage.enqueueTransaction(function getDaily() {
      let m = this.getMeasurement(AppUpdateMeasurement1.prototype.name,
                                  AppUpdateMeasurement1.prototype.version);

      let enabled = this._prefs.get("app.update.enabled", false);
      yield m.setDailyLastNumeric("enabled", enabled ? 1 : 0);

      let auto = this._prefs.get("app.update.auto", false);
      yield m.setDailyLastNumeric("autoDownload", auto ? 1 : 0);
    }.bind(this));
  },
});


function SysInfoMeasurement() {
  Metrics.Measurement.call(this);
}

SysInfoMeasurement.prototype = Object.freeze({
  __proto__: Metrics.Measurement.prototype,

  name: "sysinfo",
  version: 2,

  fields: {
    cpuCount: {type: Metrics.Storage.FIELD_LAST_NUMERIC},
    memoryMB: {type: Metrics.Storage.FIELD_LAST_NUMERIC},
    manufacturer: LAST_TEXT_FIELD,
    device: LAST_TEXT_FIELD,
    hardware: LAST_TEXT_FIELD,
    name: LAST_TEXT_FIELD,
    version: LAST_TEXT_FIELD,
    architecture: LAST_TEXT_FIELD,
    isWow64: LAST_NUMERIC_FIELD,
  },
});


this.SysInfoProvider = function SysInfoProvider() {
  Metrics.Provider.call(this);
};

SysInfoProvider.prototype = Object.freeze({
  __proto__: Metrics.Provider.prototype,

  name: "org.mozilla.sysinfo",

  measurementTypes: [SysInfoMeasurement],

  pullOnly: true,

  sysInfoFields: {
    cpucount: "cpuCount",
    memsize: "memoryMB",
    manufacturer: "manufacturer",
    device: "device",
    hardware: "hardware",
    name: "name",
    version: "version",
    arch: "architecture",
    isWow64: "isWow64",
  },

  collectConstantData: function () {
    return this.storage.enqueueTransaction(this._populateConstants.bind(this));
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

        switch (k) {
          case "memsize":
            
            value = Math.round(value / 1048576);
            break;
          case "isWow64":
            
            
            value = value ? 1 : 0;
            method = "setLastNumeric";
            break;
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
  version: 3,

  
  fields: {},

  


  getValues: function () {
    let sessions = this.provider.healthReporter.sessionRecorder;

    let fields = new Map();
    let now = new Date();
    fields.set("startDay", [now, Metrics.dateToDays(sessions.startDate)]);
    fields.set("activeTicks", [now, sessions.activeTicks]);
    fields.set("totalTime", [now, sessions.totalTime]);
    fields.set("main", [now, sessions.main]);
    fields.set("firstPaint", [now, sessions.firstPaint]);
    fields.set("sessionRestored", [now, sessions.sessionRestored]);

    return CommonUtils.laterTickResolvingPromise({
      days: new Metrics.DailyValues(),
      singular: fields,
    });
  },

  _serializeJSONSingular: function (data) {
    let result = {"_v": this.version};

    for (let [field, value] of data) {
      result[field] = value[1];
    }

    return result;
  },
});




function PreviousSessionsMeasurement() {
  Metrics.Measurement.call(this);
}

PreviousSessionsMeasurement.prototype = Object.freeze({
  __proto__: Metrics.Measurement.prototype,

  name: "previous",
  version: 3,

  fields: {
    
    cleanActiveTicks: DAILY_DISCRETE_NUMERIC_FIELD,
    cleanTotalTime: DAILY_DISCRETE_NUMERIC_FIELD,

    
    abortedActiveTicks: DAILY_DISCRETE_NUMERIC_FIELD,
    abortedTotalTime: DAILY_DISCRETE_NUMERIC_FIELD,

    
    main: DAILY_DISCRETE_NUMERIC_FIELD,
    firstPaint: DAILY_DISCRETE_NUMERIC_FIELD,
    sessionRestored: DAILY_DISCRETE_NUMERIC_FIELD,
  },
});





















this.SessionsProvider = function () {
  Metrics.Provider.call(this);
};

SessionsProvider.prototype = Object.freeze({
  __proto__: Metrics.Provider.prototype,

  name: "org.mozilla.appSessions",

  measurementTypes: [CurrentSessionMeasurement, PreviousSessionsMeasurement],

  pullOnly: true,

  collectConstantData: function () {
    let previous = this.getMeasurement("previous", 3);

    return this.storage.enqueueTransaction(this._recordAndPruneSessions.bind(this));
  },

  _recordAndPruneSessions: function () {
    this._log.info("Moving previous sessions from session recorder to storage.");
    let recorder = this.healthReporter.sessionRecorder;
    let sessions = recorder.getPreviousSessions();
    this._log.debug("Found " + Object.keys(sessions).length + " previous sessions.");

    let daily = this.getMeasurement("previous", 3);

    
    
    
    
    
    
    
    let lastRecordedSession = yield this.getState("lastSession");
    if (lastRecordedSession === null) {
      lastRecordedSession = -1;
    }
    this._log.debug("The last recorded session was #" + lastRecordedSession);

    for (let [index, session] in Iterator(sessions)) {
      if (index <= lastRecordedSession) {
        this._log.warn("Already recorded session " + index + ". Did the last " +
                       "session crash or have an issue saving the prefs file?");
        continue;
      }

      let type = session.clean ? "clean" : "aborted";
      let date = session.startDate;
      yield daily.addDailyDiscreteNumeric(type + "ActiveTicks", session.activeTicks, date);
      yield daily.addDailyDiscreteNumeric(type + "TotalTime", session.totalTime, date);

      for (let field of ["main", "firstPaint", "sessionRestored"]) {
        yield daily.addDailyDiscreteNumeric(field, session[field], date);
      }

      lastRecordedSession = index;
    }

    yield this.setState("lastSession", "" + lastRecordedSession);
    recorder.pruneOldSessions(new Date());
  },
});








function ActiveAddonsMeasurement() {
  Metrics.Measurement.call(this);

  this._serializers = {};
  this._serializers[this.SERIALIZE_JSON] = {
    singular: this._serializeJSONSingular.bind(this),
    
  };
}

ActiveAddonsMeasurement.prototype = Object.freeze({
  __proto__: Metrics.Measurement.prototype,

  name: "addons",
  version: 2,

  fields: {
    addons: LAST_TEXT_FIELD,
  },

  _serializeJSONSingular: function (data) {
    if (!data.has("addons")) {
      this._log.warn("Don't have addons info. Weird.");
      return null;
    }

    
    let result = JSON.parse(data.get("addons")[1]);
    result._v = this.version;
    return result;
  },
});







function ActivePluginsMeasurement() {
  Metrics.Measurement.call(this);

  this._serializers = {};
  this._serializers[this.SERIALIZE_JSON] = {
    singular: this._serializeJSONSingular.bind(this),
    
  };
}

ActivePluginsMeasurement.prototype = Object.freeze({
  __proto__: Metrics.Measurement.prototype,

  name: "plugins",
  version: 1,

  fields: {
    plugins: LAST_TEXT_FIELD,
  },

  _serializeJSONSingular: function (data) {
    if (!data.has("plugins")) {
      this._log.warn("Don't have plugins info. Weird.");
      return null;
    }

    
    let result = JSON.parse(data.get("plugins")[1]);
    result._v = this.version;
    return result;
  },
});

function ActiveGMPluginsMeasurement() {
  Metrics.Measurement.call(this);

  this._serializers = {};
  this._serializers[this.SERIALIZE_JSON] = {
    singular: this._serializeJSONSingular.bind(this),
  };
}

ActiveGMPluginsMeasurement.prototype = Object.freeze({
  __proto__: Metrics.Measurement.prototype,

  name: "gm-plugins",
  version: 1,

  fields: {
    "gm-plugins": LAST_TEXT_FIELD,
  },

  _serializeJSONSingular: function (data) {
    if (!data.has("gm-plugins")) {
      this._log.warn("Don't have GM plugins info. Weird.");
      return null;
    }

    let result = JSON.parse(data.get("gm-plugins")[1]);
    result._v = this.version;
    return result;
  },
});

function AddonCountsMeasurement() {
  Metrics.Measurement.call(this);
}

AddonCountsMeasurement.prototype = Object.freeze({
  __proto__: Metrics.Measurement.prototype,

  name: "counts",
  version: 2,

  fields: {
    theme: DAILY_LAST_NUMERIC_FIELD,
    lwtheme: DAILY_LAST_NUMERIC_FIELD,
    plugin: DAILY_LAST_NUMERIC_FIELD,
    extension: DAILY_LAST_NUMERIC_FIELD,
    service: DAILY_LAST_NUMERIC_FIELD,
  },
});





function AddonCountsMeasurement1() {
  Metrics.Measurement.call(this);
}

AddonCountsMeasurement1.prototype = Object.freeze({
  __proto__: Metrics.Measurement.prototype,

  name: "counts",
  version: 1,

  fields: {
    theme: DAILY_LAST_NUMERIC_FIELD,
    lwtheme: DAILY_LAST_NUMERIC_FIELD,
    plugin: DAILY_LAST_NUMERIC_FIELD,
    extension: DAILY_LAST_NUMERIC_FIELD,
  },
});


this.AddonsProvider = function () {
  Metrics.Provider.call(this);

  this._prefs = new Preferences({defaultBranch: null});
};

AddonsProvider.prototype = Object.freeze({
  __proto__: Metrics.Provider.prototype,

  
  
  
  
  
  ADDON_LISTENER_CALLBACKS: [
    "onEnabled",
    "onDisabled",
    "onInstalled",
    "onUninstalled",
  ],

  
  
  FULL_DETAIL_TYPES: [
    "extension",
    "service",
  ],

  name: "org.mozilla.addons",

  measurementTypes: [
    ActiveAddonsMeasurement,
    ActivePluginsMeasurement,
    ActiveGMPluginsMeasurement,
    AddonCountsMeasurement1,
    AddonCountsMeasurement,
  ],

  postInit: function () {
    let listener = {};

    for (let method of this.ADDON_LISTENER_CALLBACKS) {
      listener[method] = this._collectAndStoreAddons.bind(this);
    }

    this._listener = listener;
    AddonManager.addAddonListener(this._listener);

    return CommonUtils.laterTickResolvingPromise();
  },

  onShutdown: function () {
    AddonManager.removeAddonListener(this._listener);
    this._listener = null;

    return CommonUtils.laterTickResolvingPromise();
  },

  collectConstantData: function () {
    return this._collectAndStoreAddons();
  },

  _collectAndStoreAddons: function () {
    let deferred = Promise.defer();

    AddonManager.getAllAddons(function onAllAddons(allAddons) {
      let data;
      let addonsField;
      let pluginsField;
      let gmPluginsField;
      try {
        data = this._createDataStructure(allAddons);
        addonsField = JSON.stringify(data.addons);
        pluginsField = JSON.stringify(data.plugins);
        gmPluginsField = JSON.stringify(data.gmPlugins);
      } catch (ex) {
        this._log.warn("Exception when populating add-ons data structure: " +
                       CommonUtils.exceptionStr(ex));
        deferred.reject(ex);
        return;
      }

      let now = new Date();
      let addons = this.getMeasurement("addons", 2);
      let plugins = this.getMeasurement("plugins", 1);
      let gmPlugins = this.getMeasurement("gm-plugins", 1);
      let counts = this.getMeasurement(AddonCountsMeasurement.prototype.name,
                                       AddonCountsMeasurement.prototype.version);

      this.enqueueStorageOperation(function storageAddons() {
        for (let type in data.counts) {
          try {
            counts.fieldID(type);
          } catch (ex) {
            this._log.warn("Add-on type without field: " + type);
            continue;
          }

          counts.setDailyLastNumeric(type, data.counts[type], now);
        }

        return addons.setLastText("addons", addonsField).then(
          function onSuccess() {
            return plugins.setLastText("plugins", pluginsField).then(
              function onSuccess() {
                return gmPlugins.setLastText("gm-plugins", gmPluginsField).then(
                  function onSuccess() {
                    deferred.resolve();
                  },
                  function onError(error) {
                    deferred.reject(error);
                  });
              },
              function onError(error) { deferred.reject(error); }
            );
          },
          function onError(error) { deferred.reject(error); }
        );
      }.bind(this));
    }.bind(this));

    return deferred.promise;
  },

  COPY_ADDON_FIELDS: [
    "userDisabled",
    "appDisabled",
    "name",
    "version",
    "type",
    "scope",
    "description",
    "foreignInstall",
    "hasBinaryComponents",
  ],

  COPY_PLUGIN_FIELDS: [
    "name",
    "version",
    "description",
    "blocklisted",
    "disabled",
    "clicktoplay",
  ],

  _createDataStructure: function (addons) {
    let data = {
      addons: {},
      plugins: {},
      gmPlugins: {},
      counts: {}
    };

    for (let addon of addons) {
      let type = addon.type;

      
      if (addon.type == "plugin") {
        if (addon.isGMPlugin) {
          data.gmPlugins[addon.id] = {
            version: addon.version,
            userDisabled: addon.userDisabled,
            applyBackgroundUpdates: addon.applyBackgroundUpdates,
          };
        }
        continue;
      }

      data.counts[type] = (data.counts[type] || 0) + 1;

      if (this.FULL_DETAIL_TYPES.indexOf(addon.type) == -1) {
        continue;
      }

      let obj = {};
      for (let field of this.COPY_ADDON_FIELDS) {
        obj[field] = addon[field];
      }

      if (addon.installDate) {
        obj.installDay = this._dateToDays(addon.installDate);
      }

      if (addon.updateDate) {
        obj.updateDay = this._dateToDays(addon.updateDate);
      }

      data.addons[addon.id] = obj;
    }

    let pluginTags = Cc["@mozilla.org/plugin/host;1"].
                       getService(Ci.nsIPluginHost).
                       getPluginTags({});

    for (let tag of pluginTags) {
      let obj = {
        mimeTypes: tag.getMimeTypes({}),
      };

      for (let field of this.COPY_PLUGIN_FIELDS) {
        obj[field] = tag[field];
      }

      
      let id = tag.filename + ":" + tag.name + ":" + tag.version + ":"
               + tag.description;
      data.plugins[id] = obj;
    }

    data.counts["plugin"] = pluginTags.length;

    return data;
  },
});

#ifdef MOZ_CRASHREPORTER

function DailyCrashesMeasurement1() {
  Metrics.Measurement.call(this);
}

DailyCrashesMeasurement1.prototype = Object.freeze({
  __proto__: Metrics.Measurement.prototype,

  name: "crashes",
  version: 1,

  fields: {
    pending: DAILY_COUNTER_FIELD,
    submitted: DAILY_COUNTER_FIELD,
  },
});

function DailyCrashesMeasurement2() {
  Metrics.Measurement.call(this);
}

DailyCrashesMeasurement2.prototype = Object.freeze({
  __proto__: Metrics.Measurement.prototype,

  name: "crashes",
  version: 2,

  fields: {
    mainCrash: DAILY_LAST_NUMERIC_FIELD,
  },
});

function DailyCrashesMeasurement3() {
  Metrics.Measurement.call(this);
}

DailyCrashesMeasurement3.prototype = Object.freeze({
  __proto__: Metrics.Measurement.prototype,

  name: "crashes",
  version: 3,

  fields: {
    "main-crash": DAILY_LAST_NUMERIC_FIELD,
    "main-hang": DAILY_LAST_NUMERIC_FIELD,
    "content-crash": DAILY_LAST_NUMERIC_FIELD,
    "content-hang": DAILY_LAST_NUMERIC_FIELD,
    "plugin-crash": DAILY_LAST_NUMERIC_FIELD,
    "plugin-hang": DAILY_LAST_NUMERIC_FIELD,
  },
});

function DailyCrashesMeasurement4() {
  Metrics.Measurement.call(this);
}

DailyCrashesMeasurement4.prototype = Object.freeze({
  __proto__: Metrics.Measurement.prototype,

  name: "crashes",
  version: 4,

  fields: {
    "main-crash": DAILY_LAST_NUMERIC_FIELD,
    "main-crash-submission-succeeded": DAILY_LAST_NUMERIC_FIELD,
    "main-crash-submission-failed": DAILY_LAST_NUMERIC_FIELD,
    "main-hang": DAILY_LAST_NUMERIC_FIELD,
    "main-hang-submission-succeeded": DAILY_LAST_NUMERIC_FIELD,
    "main-hang-submission-failed": DAILY_LAST_NUMERIC_FIELD,
    "content-crash": DAILY_LAST_NUMERIC_FIELD,
    "content-crash-submission-succeeded": DAILY_LAST_NUMERIC_FIELD,
    "content-crash-submission-failed": DAILY_LAST_NUMERIC_FIELD,
    "content-hang": DAILY_LAST_NUMERIC_FIELD,
    "content-hang-submission-succeeded": DAILY_LAST_NUMERIC_FIELD,
    "content-hang-submission-failed": DAILY_LAST_NUMERIC_FIELD,
    "plugin-crash": DAILY_LAST_NUMERIC_FIELD,
    "plugin-crash-submission-succeeded": DAILY_LAST_NUMERIC_FIELD,
    "plugin-crash-submission-failed": DAILY_LAST_NUMERIC_FIELD,
    "plugin-hang": DAILY_LAST_NUMERIC_FIELD,
    "plugin-hang-submission-succeeded": DAILY_LAST_NUMERIC_FIELD,
    "plugin-hang-submission-failed": DAILY_LAST_NUMERIC_FIELD,
  },
});

function DailyCrashesMeasurement5() {
  Metrics.Measurement.call(this);
}

DailyCrashesMeasurement5.prototype = Object.freeze({
  __proto__: Metrics.Measurement.prototype,

  name: "crashes",
  version: 5,

  fields: {
    "main-crash": DAILY_LAST_NUMERIC_FIELD,
    "main-crash-submission-succeeded": DAILY_LAST_NUMERIC_FIELD,
    "main-crash-submission-failed": DAILY_LAST_NUMERIC_FIELD,
    "main-hang": DAILY_LAST_NUMERIC_FIELD,
    "main-hang-submission-succeeded": DAILY_LAST_NUMERIC_FIELD,
    "main-hang-submission-failed": DAILY_LAST_NUMERIC_FIELD,
    "content-crash": DAILY_LAST_NUMERIC_FIELD,
    "content-crash-submission-succeeded": DAILY_LAST_NUMERIC_FIELD,
    "content-crash-submission-failed": DAILY_LAST_NUMERIC_FIELD,
    "content-hang": DAILY_LAST_NUMERIC_FIELD,
    "content-hang-submission-succeeded": DAILY_LAST_NUMERIC_FIELD,
    "content-hang-submission-failed": DAILY_LAST_NUMERIC_FIELD,
    "plugin-crash": DAILY_LAST_NUMERIC_FIELD,
    "plugin-crash-submission-succeeded": DAILY_LAST_NUMERIC_FIELD,
    "plugin-crash-submission-failed": DAILY_LAST_NUMERIC_FIELD,
    "plugin-hang": DAILY_LAST_NUMERIC_FIELD,
    "plugin-hang-submission-succeeded": DAILY_LAST_NUMERIC_FIELD,
    "plugin-hang-submission-failed": DAILY_LAST_NUMERIC_FIELD,
    "gmplugin-crash": DAILY_LAST_NUMERIC_FIELD,
    "gmplugin-crash-submission-succeeded": DAILY_LAST_NUMERIC_FIELD,
    "gmplugin-crash-submission-failed": DAILY_LAST_NUMERIC_FIELD,
  },
});

function DailyCrashesMeasurement6() {
  Metrics.Measurement.call(this);
}

DailyCrashesMeasurement6.prototype = Object.freeze({
  __proto__: Metrics.Measurement.prototype,

  name: "crashes",
  version: 6,

  fields: {
    "main-crash": DAILY_LAST_NUMERIC_FIELD,
    "main-crash-oom": DAILY_LAST_NUMERIC_FIELD,
    "main-crash-submission-succeeded": DAILY_LAST_NUMERIC_FIELD,
    "main-crash-submission-failed": DAILY_LAST_NUMERIC_FIELD,
    "main-hang": DAILY_LAST_NUMERIC_FIELD,
    "main-hang-submission-succeeded": DAILY_LAST_NUMERIC_FIELD,
    "main-hang-submission-failed": DAILY_LAST_NUMERIC_FIELD,
    "content-crash": DAILY_LAST_NUMERIC_FIELD,
    "content-crash-submission-succeeded": DAILY_LAST_NUMERIC_FIELD,
    "content-crash-submission-failed": DAILY_LAST_NUMERIC_FIELD,
    "content-hang": DAILY_LAST_NUMERIC_FIELD,
    "content-hang-submission-succeeded": DAILY_LAST_NUMERIC_FIELD,
    "content-hang-submission-failed": DAILY_LAST_NUMERIC_FIELD,
    "plugin-crash": DAILY_LAST_NUMERIC_FIELD,
    "plugin-crash-submission-succeeded": DAILY_LAST_NUMERIC_FIELD,
    "plugin-crash-submission-failed": DAILY_LAST_NUMERIC_FIELD,
    "plugin-hang": DAILY_LAST_NUMERIC_FIELD,
    "plugin-hang-submission-succeeded": DAILY_LAST_NUMERIC_FIELD,
    "plugin-hang-submission-failed": DAILY_LAST_NUMERIC_FIELD,
    "gmplugin-crash": DAILY_LAST_NUMERIC_FIELD,
    "gmplugin-crash-submission-succeeded": DAILY_LAST_NUMERIC_FIELD,
    "gmplugin-crash-submission-failed": DAILY_LAST_NUMERIC_FIELD,
  },
});

this.CrashesProvider = function () {
  Metrics.Provider.call(this);

  
  this._manager = Services.crashmanager;
};

CrashesProvider.prototype = Object.freeze({
  __proto__: Metrics.Provider.prototype,

  name: "org.mozilla.crashes",

  measurementTypes: [
    DailyCrashesMeasurement1,
    DailyCrashesMeasurement2,
    DailyCrashesMeasurement3,
    DailyCrashesMeasurement4,
    DailyCrashesMeasurement5,
    DailyCrashesMeasurement6,
  ],

  pullOnly: true,

  collectDailyData: function () {
    return this.storage.enqueueTransaction(this._populateCrashCounts.bind(this));
  },

  _populateCrashCounts: function () {
    this._log.info("Grabbing crash counts from crash manager.");
    let crashCounts = yield this._manager.getCrashCountsByDay();

    
    
    
    
    let crashes = yield this._manager.getCrashes();
    for (let crash of crashes) {
      for (let [submissionID, submission] of crash.submissions) {
        if (!submission.responseDate) {
          continue;
        }

        let day = Metrics.dateToDays(submission.responseDate);
        if (!crashCounts.has(day)) {
          crashCounts.set(day, new Map());
        }

        let succeeded =
          submission.result == this._manager.SUBMISSION_RESULT_OK;
        let type = crash.type + "-submission-" + (succeeded ? "succeeded" :
                                                              "failed");

        let count = (crashCounts.get(day).get(type) || 0) + 1;
        crashCounts.get(day).set(type, count);
      }
    }

    let m = this.getMeasurement("crashes", 6);
    let fields = DailyCrashesMeasurement6.prototype.fields;

    for (let [day, types] of crashCounts) {
      let date = Metrics.daysToDate(day);
      for (let [type, count] of types) {
        if (!(type in fields)) {
          this._log.warn("Unknown crash type encountered: " + type);
          continue;
        }

        yield m.setDailyLastNumeric(type, count, date);
      }
    }
  },
});

#endif












function UpdateHotfixMeasurement1() {
  Metrics.Measurement.call(this);
}

UpdateHotfixMeasurement1.prototype = Object.freeze({
  __proto__: Metrics.Measurement.prototype,

  name: "update",
  version: 1,

  hotfixFieldTypes: {
    "upgradedFrom": Metrics.Storage.FIELD_LAST_TEXT,
    "uninstallReason": Metrics.Storage.FIELD_LAST_TEXT,
    "downloadAttempts": Metrics.Storage.FIELD_LAST_NUMERIC,
    "downloadFailures": Metrics.Storage.FIELD_LAST_NUMERIC,
    "installAttempts": Metrics.Storage.FIELD_LAST_NUMERIC,
    "installFailures": Metrics.Storage.FIELD_LAST_NUMERIC,
    "notificationsShown": Metrics.Storage.FIELD_LAST_NUMERIC,
  },

  fields: { },

  
  
  shouldIncludeField: function (name) {
    return name.includes(".");
  },

  fieldType: function (name) {
    for (let known in this.hotfixFieldTypes) {
      if (name.endsWith(known)) {
        return this.hotfixFieldTypes[known];
      }
    }

    return Metrics.Measurement.prototype.fieldType.call(this, name);
  },
});

this.HotfixProvider = function () {
  Metrics.Provider.call(this);
};

HotfixProvider.prototype = Object.freeze({
  __proto__: Metrics.Provider.prototype,

  name: "org.mozilla.hotfix",
  measurementTypes: [
    UpdateHotfixMeasurement1,
  ],

  pullOnly: true,

  collectDailyData: function () {
    return this.storage.enqueueTransaction(this._populateHotfixData.bind(this));
  },

  _populateHotfixData: function* () {
    let m = this.getMeasurement("update", 1);

    
    
    
    
    
    let files = [
        ["v20140527", OS.Path.join(OS.Constants.Path.profileDir,
                                   "hotfix.v20140527.01.json")],
    ];

    let it = new OS.File.DirectoryIterator(OS.Constants.Path.profileDir);
    try {
      yield it.forEach((e, index, it) => {
        let m = e.name.match(/^updateHotfix\.([a-zA-Z0-9]+)\.json$/);
        if (m) {
          files.push([m[1], e.path]);
        }
      });
    } finally {
      it.close();
    }

    let decoder = new TextDecoder();
    for (let e of files) {
      let [version, path] = e;
      let p;
      try {
        let data = yield OS.File.read(path);
        p = JSON.parse(decoder.decode(data));
      } catch (ex if ex instanceof OS.File.Error && ex.becauseNoSuchFile) {
        continue;
      } catch (ex) {
        this._log.warn("Error loading update hotfix payload: " + ex.message);
      }

      
      try {
        for (let k in m.hotfixFieldTypes) {
          if (!(k in p)) {
            continue;
          }

          let value = p[k];
          if (value === null && k == "uninstallReason") {
            value = "STILL_INSTALLED";
          }

          let field = version + "." + k;
          let fieldType;
          let storageOp;
          switch (typeof(value)) {
            case "string":
              fieldType = this.storage.FIELD_LAST_TEXT;
              storageOp = "setLastTextFromFieldID";
              break;
            case "number":
              fieldType = this.storage.FIELD_LAST_NUMERIC;
              storageOp = "setLastNumericFromFieldID";
              break;
            default:
              this._log.warn("Unknown value in hotfix state: " + k + "=" + value);
              continue;
          }

          if (this.storage.hasFieldFromMeasurement(m.id, field, fieldType)) {
            let fieldID = this.storage.fieldIDFromMeasurement(m.id, field);
            yield this.storage[storageOp](fieldID, value);
          } else {
            let fieldID = yield this.storage.registerField(m.id, field,
                                                           fieldType);
            yield this.storage[storageOp](fieldID, value);
          }
        }

      } catch (ex) {
        this._log.warn("Error processing update hotfix data: " + ex);
      }
    }
  },
});




function PlacesMeasurement() {
  Metrics.Measurement.call(this);
}

PlacesMeasurement.prototype = Object.freeze({
  __proto__: Metrics.Measurement.prototype,

  name: "places",
  version: 1,

  fields: {
    pages: DAILY_LAST_NUMERIC_FIELD,
    bookmarks: DAILY_LAST_NUMERIC_FIELD,
  },
});





this.PlacesProvider = function () {
  Metrics.Provider.call(this);
};

PlacesProvider.prototype = Object.freeze({
  __proto__: Metrics.Provider.prototype,

  name: "org.mozilla.places",

  measurementTypes: [PlacesMeasurement],

  collectDailyData: function () {
    return this.storage.enqueueTransaction(this._collectData.bind(this));
  },

  _collectData: function () {
    let now = new Date();
    let data = yield this._getDailyValues();

    let m = this.getMeasurement("places", 1);

    yield m.setDailyLastNumeric("pages", data.PLACES_PAGES_COUNT);
    yield m.setDailyLastNumeric("bookmarks", data.PLACES_BOOKMARKS_COUNT);
  },

  _getDailyValues: function () {
    let deferred = Promise.defer();

    PlacesDBUtils.telemetry(null, function onResult(data) {
      deferred.resolve(data);
    });

    return deferred.promise;
  },
});

function SearchCountMeasurement1() {
  Metrics.Measurement.call(this);
}

SearchCountMeasurement1.prototype = Object.freeze({
  __proto__: Metrics.Measurement.prototype,

  name: "counts",
  version: 1,

  
  
  fields: {
    "amazon.com.abouthome": DAILY_COUNTER_FIELD,
    "amazon.com.contextmenu": DAILY_COUNTER_FIELD,
    "amazon.com.searchbar": DAILY_COUNTER_FIELD,
    "amazon.com.urlbar": DAILY_COUNTER_FIELD,
    "bing.abouthome": DAILY_COUNTER_FIELD,
    "bing.contextmenu": DAILY_COUNTER_FIELD,
    "bing.searchbar": DAILY_COUNTER_FIELD,
    "bing.urlbar": DAILY_COUNTER_FIELD,
    "google.abouthome": DAILY_COUNTER_FIELD,
    "google.contextmenu": DAILY_COUNTER_FIELD,
    "google.searchbar": DAILY_COUNTER_FIELD,
    "google.urlbar": DAILY_COUNTER_FIELD,
    "yahoo.abouthome": DAILY_COUNTER_FIELD,
    "yahoo.contextmenu": DAILY_COUNTER_FIELD,
    "yahoo.searchbar": DAILY_COUNTER_FIELD,
    "yahoo.urlbar": DAILY_COUNTER_FIELD,
    "other.abouthome": DAILY_COUNTER_FIELD,
    "other.contextmenu": DAILY_COUNTER_FIELD,
    "other.searchbar": DAILY_COUNTER_FIELD,
    "other.urlbar": DAILY_COUNTER_FIELD,
  },
});












function SearchCountMeasurementBase() {
  this._fieldSpecs = {};
  Metrics.Measurement.call(this);
}

SearchCountMeasurementBase.prototype = Object.freeze({
  __proto__: Metrics.Measurement.prototype,


  
  get fields() {
    return this._fieldSpecs;
  },

  











  shouldIncludeField: function (name) {
    return name.includes(".");
  },

  



  fieldType: function (name) {
    if (name in this.fields) {
      return this.fields[name].type;
    }

    
    return Metrics.Storage.FIELD_DAILY_COUNTER;
  },

  SOURCES: [
    "abouthome",
    "contextmenu",
    "newtab",
    "searchbar",
    "urlbar",
  ],
});

function SearchCountMeasurement2() {
  SearchCountMeasurementBase.call(this);
}

SearchCountMeasurement2.prototype = Object.freeze({
  __proto__: SearchCountMeasurementBase.prototype,
  name: "counts",
  version: 2,
});

function SearchCountMeasurement3() {
  SearchCountMeasurementBase.call(this);
}

SearchCountMeasurement3.prototype = Object.freeze({
  __proto__: SearchCountMeasurementBase.prototype,
  name: "counts",
  version: 3,

  getEngines: function () {
    return Services.search.getEngines();
  },

  getEngineID: function (engine) {
    if (!engine) {
      return "other";
    }
    if (engine.identifier) {
      return engine.identifier;
    }
    return "other-" + engine.name;
  },
});

function SearchEnginesMeasurement1() {
  Metrics.Measurement.call(this);
}

SearchEnginesMeasurement1.prototype = Object.freeze({
  __proto__: Metrics.Measurement.prototype,

  name: "engines",
  version: 1,

  fields: {
    default: DAILY_LAST_TEXT_FIELD,
  },
});

this.SearchesProvider = function () {
  Metrics.Provider.call(this);

  this._prefs = new Preferences({defaultBranch: null});
};

this.SearchesProvider.prototype = Object.freeze({
  __proto__: Metrics.Provider.prototype,

  name: "org.mozilla.searches",
  measurementTypes: [
    SearchCountMeasurement1,
    SearchCountMeasurement2,
    SearchCountMeasurement3,
    SearchEnginesMeasurement1,
  ],

  


  preInit: function (storage) {
    
    let deferred = Promise.defer();
    Services.search.init(function onInitComplete () {
      deferred.resolve();
    });
    return deferred.promise;
  },

  collectDailyData: function () {
    return this.storage.enqueueTransaction(function getDaily() {
      let m = this.getMeasurement(SearchEnginesMeasurement1.prototype.name,
                                  SearchEnginesMeasurement1.prototype.version);

      let engine;
      try {
        engine = Services.search.defaultEngine;
      } catch (e) {}
      let name;

      if (!engine) {
        name = "NONE";
      } else if (engine.identifier) {
        name = engine.identifier;
      } else if (engine.name) {
        name = "other-" + engine.name;
      } else {
        name = "UNDEFINED";
      }

      yield m.setDailyLastText("default", name);
    }.bind(this));
  },

  











  recordSearch: function (engine, source) {
    let m = this.getMeasurement("counts", 3);

    if (m.SOURCES.indexOf(source) == -1) {
      throw new Error("Unknown source for search: " + source);
    }

    let field = m.getEngineID(engine) + "." + source;
    if (this.storage.hasFieldFromMeasurement(m.id, field,
                                             this.storage.FIELD_DAILY_COUNTER)) {
      let fieldID = this.storage.fieldIDFromMeasurement(m.id, field);
      return this.enqueueStorageOperation(function recordSearchKnownField() {
        return this.storage.incrementDailyCounterFromFieldID(fieldID);
      }.bind(this));
    }

    
    return this.enqueueStorageOperation(function recordFieldAndSearch() {
      
      return Task.spawn(function () {
        let fieldID = yield this.storage.registerField(m.id, field,
                                                       this.storage.FIELD_DAILY_COUNTER);
        yield this.storage.incrementDailyCounterFromFieldID(fieldID);
      }.bind(this));
    }.bind(this));
  },
});

function HealthReportSubmissionMeasurement1() {
  Metrics.Measurement.call(this);
}

HealthReportSubmissionMeasurement1.prototype = Object.freeze({
  __proto__: Metrics.Measurement.prototype,

  name: "submissions",
  version: 1,

  fields: {
    firstDocumentUploadAttempt: DAILY_COUNTER_FIELD,
    continuationUploadAttempt: DAILY_COUNTER_FIELD,
    uploadSuccess: DAILY_COUNTER_FIELD,
    uploadTransportFailure: DAILY_COUNTER_FIELD,
    uploadServerFailure: DAILY_COUNTER_FIELD,
    uploadClientFailure: DAILY_COUNTER_FIELD,
  },
});

function HealthReportSubmissionMeasurement2() {
  Metrics.Measurement.call(this);
}

HealthReportSubmissionMeasurement2.prototype = Object.freeze({
  __proto__: Metrics.Measurement.prototype,

  name: "submissions",
  version: 2,

  fields: {
    firstDocumentUploadAttempt: DAILY_COUNTER_FIELD,
    continuationUploadAttempt: DAILY_COUNTER_FIELD,
    uploadSuccess: DAILY_COUNTER_FIELD,
    uploadTransportFailure: DAILY_COUNTER_FIELD,
    uploadServerFailure: DAILY_COUNTER_FIELD,
    uploadClientFailure: DAILY_COUNTER_FIELD,
    uploadAlreadyInProgress: DAILY_COUNTER_FIELD,
  },
});

this.HealthReportProvider = function () {
  Metrics.Provider.call(this);
}

HealthReportProvider.prototype = Object.freeze({
  __proto__: Metrics.Provider.prototype,

  name: "org.mozilla.healthreport",

  measurementTypes: [
    HealthReportSubmissionMeasurement1,
    HealthReportSubmissionMeasurement2,
  ],

  recordEvent: function (event, date=new Date()) {
    let m = this.getMeasurement("submissions", 2);
    return this.enqueueStorageOperation(function recordCounter() {
      return m.incrementDailyCounter(event, date);
    });
  },
});
