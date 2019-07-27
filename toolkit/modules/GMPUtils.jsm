



"use strict";

const {classes: Cc, interfaces: Ci, results: Cr, utils: Cu, manager: Cm} =
  Components;

this.EXPORTED_SYMBOLS = [ "EME_ADOBE_ID",
                          "GMP_PLUGIN_IDS",
                          "GMPPrefs",
                          "GMPUtils",
                          "OPEN_H264_ID" ];

Cu.import("resource://gre/modules/Preferences.jsm");
Cu.import("resource://gre/modules/Services.jsm");


const OPEN_H264_ID  = "gmp-gmpopenh264";
const EME_ADOBE_ID  = "gmp-eme-adobe";
const GMP_PLUGIN_IDS = [ OPEN_H264_ID, EME_ADOBE_ID ];

this.GMPUtils = {
  





  isPluginHidden: function(aPlugin) {
    if (aPlugin.isEME) {
      if (this._isPluginSupported(aPlugin) ||
          this._isPluginForcedVisible(aPlugin)) {
        return !GMPPrefs.get(GMPPrefs.KEY_EME_ENABLED, true);
      } else {
        return true;
      }
    }
    return false;
  },

  




  _isPluginSupported: function(aPlugin) {
    if (aPlugin.id == EME_ADOBE_ID) {
      if (Services.appinfo.OS == "WINNT") {
        return Services.sysinfo.getPropertyAsInt32("version") >= 6;
      } else {
        return false;
      }
    }
    return true;
  },

  





  _isPluginForcedVisible: function(aPlugin) {
    return GMPPrefs.get(GMPPrefs.KEY_PLUGIN_FORCEVISIBLE, false, aPlugin.id);
  },
};




this.GMPPrefs = {
  KEY_EME_ENABLED:              "media.eme.enabled",
  KEY_PLUGIN_ENABLED:           "media.{0}.enabled",
  KEY_PLUGIN_LAST_UPDATE:       "media.{0}.lastUpdate",
  KEY_PLUGIN_VERSION:           "media.{0}.version",
  KEY_PLUGIN_AUTOUPDATE:        "media.{0}.autoupdate",
  KEY_PLUGIN_FORCEVISIBLE:      "media.{0}.forcevisible",
  KEY_PLUGIN_TRIAL_CREATE:      "media.{0}.trial-create",
  KEY_URL:                      "media.gmp-manager.url",
  KEY_URL_OVERRIDE:             "media.gmp-manager.url.override",
  KEY_CERT_CHECKATTRS:          "media.gmp-manager.cert.checkAttributes",
  KEY_CERT_REQUIREBUILTIN:      "media.gmp-manager.cert.requireBuiltIn",
  KEY_UPDATE_LAST_CHECK:        "media.gmp-manager.lastCheck",
  KEY_SECONDS_BETWEEN_CHECKS:   "media.gmp-manager.secondsBetweenChecks",
  KEY_APP_DISTRIBUTION:         "distribution.id",
  KEY_APP_DISTRIBUTION_VERSION: "distribution.version",
  KEY_BUILDID:                  "media.gmp-manager.buildID",
  KEY_CERTS_BRANCH:             "media.gmp-manager.certs.",
  KEY_PROVIDER_ENABLED:         "media.gmp-provider.enabled",
  KEY_LOG_BASE:                 "media.gmp.log.",
  KEY_LOGGING_LEVEL:            "media.gmp.log.level",
  KEY_LOGGING_DUMP:             "media.gmp.log.dump",

  






  get: function(aKey, aDefaultValue, aPlugin) {
    if (aKey === this.KEY_APP_DISTRIBUTION ||
        aKey === this.KEY_APP_DISTRIBUTION_VERSION) {
      let prefValue = "default";
      try {
        prefValue = Services.prefs.getDefaultBranch(null).getCharPref(aKey);
      } catch (e) {
        
      }
      return prefValue;
    }
    return Preferences.get(this.getPrefKey(aKey, aPlugin), aDefaultValue);
  },

  





  set: function(aKey, aVal, aPlugin) {
    Preferences.set(this.getPrefKey(aKey, aPlugin), aVal);
  },

  






  isSet: function(aKey, aPlugin) {
    return Preferences.isSet(this.getPrefKey(aKey, aPlugin));
  },

  





  reset: function(aKey, aPlugin) {
    Preferences.reset(this.getPrefKey(aKey, aPlugin));
  },

  





  getPrefKey: function(aKey, aPlugin) {
    return aKey.replace("{0}", aPlugin || "");
  },
};