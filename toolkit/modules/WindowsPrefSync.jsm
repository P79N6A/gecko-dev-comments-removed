



'use strict';

this.EXPORTED_SYMBOLS = [];

#ifdef XP_WIN
#ifdef MOZ_METRO

const {classes: Cc, interfaces: Ci, results: Cr, utils: Cu, manager: Cm} =
  Components;
const PREF_BASE_KEY = "Software\\Mozilla\\Firefox\\Metro\\Prefs\\";

Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://gre/modules/Services.jsm");

this.EXPORTED_SYMBOLS = [ "WindowsPrefSync" ];





this.WindowsPrefSync = {
  init: function() {
    this.pullSharedPrefs();
    this.prefListToPush.forEach(function(prefName) {
      this.pushSharedPref(prefName);
      Services.prefs.addObserver(prefName, this, false);
    }, this);
  },

  uninit: function() {
    this.prefListToPush.forEach(function(prefName) {
        Services.prefs.removeObserver(prefName, this);
    }, this);
  },

  



  get prefListToPush() {
    return !Services.metro.immersive ? this.desktopControlledPrefs :
      this.metroControlledPrefs;
  },

  



  get prefListToPull() {
    return Services.metro.immersive ? this.desktopControlledPrefs :
      this.metroControlledPrefs;
  },

  









  desktopControlledPrefs: ["app.update.auto",
    "app.update.enabled",
    "app.update.service.enabled",
    "app.update.metro.enabled",
    "browser.sessionstore.resume_session_once"],

  






  metroControlledPrefs: ["browser.sessionstore.resume_session_once"],

  



  observe: function (aSubject, aTopic, aPrefName) {
    if (aTopic != "nsPref:changed")
      return;

    this.pushSharedPref(aPrefName);
  },

  



  pushSharedPref : function(aPrefName) {
    let registry = Cc["@mozilla.org/windows-registry-key;1"].
      createInstance(Ci.nsIWindowsRegKey);
    try {
      var prefType = Services.prefs.getPrefType(aPrefName);
      let prefFunc;
      if (prefType == Ci.nsIPrefBranch.PREF_INT)
        prefFunc = "getIntPref";
      else if (prefType == Ci.nsIPrefBranch.PREF_BOOL)
        prefFunc = "getBoolPref";
      else if (prefType == Ci.nsIPrefBranch.PREF_STRING)
        prefFunc = "getCharPref";
      else
        throw "Unsupported pref type";

      let prefValue = Services.prefs[prefFunc](aPrefName);
      registry.create(Ci.nsIWindowsRegKey.ROOT_KEY_CURRENT_USER,
        PREF_BASE_KEY + prefType,
        Ci.nsIWindowsRegKey.ACCESS_WRITE);
      
      
      registry.writeStringValue(aPrefName, prefValue);
    } catch (ex) {
      Cu.reportError("Couldn't push pref " + aPrefName + ": " + ex);
    } finally {
      registry.close();
    }
  },

  


  pullSharedPrefs: function() {
    function pullSharedPrefType(prefType, prefFunc) {
      try {
        registry.create(Ci.nsIWindowsRegKey.ROOT_KEY_CURRENT_USER,
          PREF_BASE_KEY + prefType,
          Ci.nsIWindowsRegKey.ACCESS_ALL);
        for (let i = 0; i < registry.valueCount; i++) {
          let prefName = registry.getValueName(i);
          let prefValue = registry.readStringValue(prefName);
          if (prefType == Ci.nsIPrefBranch.PREF_BOOL) {
            prefValue = prefValue == "true";
          }
          if (self.prefListToPull.indexOf(prefName) != -1) {
            Services.prefs[prefFunc](prefName, prefValue);
          }
        }
      } catch (ex) {
        dump("Could not pull for prefType " + prefType + ": " + ex + "\n");
      } finally {
        registry.close();
      }
    }
    let self = this;
    let registry = Cc["@mozilla.org/windows-registry-key;1"].
      createInstance(Ci.nsIWindowsRegKey);
    pullSharedPrefType(Ci.nsIPrefBranch.PREF_INT, "setIntPref");
    pullSharedPrefType(Ci.nsIPrefBranch.PREF_BOOL, "setBoolPref");
    pullSharedPrefType(Ci.nsIPrefBranch.PREF_STRING, "setCharPref");
  }
};
#endif
#endif
