













"use strict";

this.EXPORTED_SYMBOLS = [
  "AppInfoProvider"
];

const {classes: Cc, interfaces: Ci, utils: Cu} = Components;

Cu.import("resource://gre/modules/Services.jsm");
Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://gre/modules/services/metrics/dataprovider.jsm");
Cu.import("resource://services-common/preferences.js");
Cu.import("resource://services-common/utils.js");


const REQUIRED_STRING_TYPE = {type: "TYPE_STRING"};

XPCOMUtils.defineLazyModuleGetter(this, "UpdateChannel",
                                  "resource://gre/modules/UpdateChannel.jsm");







function AppInfoMeasurement() {
  MetricsMeasurement.call(this, "appinfo", 1);
}

AppInfoMeasurement.prototype = {
  __proto__: MetricsMeasurement.prototype,

  fields: {
    vendor: REQUIRED_STRING_TYPE,
    name: REQUIRED_STRING_TYPE,
    id: REQUIRED_STRING_TYPE,
    version: REQUIRED_STRING_TYPE,
    appBuildID: REQUIRED_STRING_TYPE,
    platformVersion: REQUIRED_STRING_TYPE,
    platformBuildID: REQUIRED_STRING_TYPE,
    os: REQUIRED_STRING_TYPE,
    xpcomabi: REQUIRED_STRING_TYPE,
    updateChannel: REQUIRED_STRING_TYPE,
    distributionID: REQUIRED_STRING_TYPE,
    distributionVersion: REQUIRED_STRING_TYPE,
    hotfixVersion: REQUIRED_STRING_TYPE,
    locale: REQUIRED_STRING_TYPE,
  },
};

Object.freeze(AppInfoMeasurement.prototype);


this.AppInfoProvider = function AppInfoProvider() {
  MetricsProvider.call(this, "app-info");

  this._prefs = new Preferences({defaultBranch: null});
}
AppInfoProvider.prototype = {
  __proto__: MetricsProvider.prototype,

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

  collectConstantMeasurements: function collectConstantMeasurements() {
    let result = this.createResult();
    result.expectMeasurement("appinfo");

    result.populate = this._populateConstants.bind(this);
    return result;
  },

  _populateConstants: function _populateConstants(result) {
    result.addMeasurement(new AppInfoMeasurement());

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
        result.setValue("appinfo", k, ai[v]);
      } catch (ex) {
        this._log.warn("Error obtaining Services.appinfo." + v);
        result.addError(ex);
      }
    }

    try {
      result.setValue("appinfo", "updateChannel", UpdateChannel.get());
    } catch (ex) {
      this._log.warn("Could not obtain update channel: " +
                     CommonUtils.exceptionStr(ex));
      result.addError(ex);
    }

    result.setValue("appinfo", "distributionID", this._prefs.get("distribution.id", ""));
    result.setValue("appinfo", "distributionVersion", this._prefs.get("distribution.version", ""));
    result.setValue("appinfo", "hotfixVersion", this._prefs.get("extensions.hotfix.lastVersion", ""));

    try {
      let locale = Cc["@mozilla.org/chrome/chrome-registry;1"]
                     .getService(Ci.nsIXULChromeRegistry)
                     .getSelectedLocale("global");
      result.setValue("appinfo", "locale", locale);
    } catch (ex) {
      this._log.warn("Could not obtain application locale: " +
                     CommonUtils.exceptionStr(ex));
      result.addError(ex);
    }

    result.finish();
  },
};

Object.freeze(AppInfoProvider.prototype);

