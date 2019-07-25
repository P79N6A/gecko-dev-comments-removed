



































 




var EXPORTED_SYMBOLS = ["Preference"];

const CC = Components.classes;
const CI = Components.interfaces;
const CU = Components.utils;
const WEAVE_PREF_PREFIX = "services.sync.prefs.sync.";

let prefs = CC["@mozilla.org/preferences-service;1"]
           .getService(CI.nsIPrefBranch);

CU.import("resource://tps/logger.jsm");






function Preference (props) {
  Logger.AssertTrue("name" in props && "value" in props,
    "Preference must have both name and value");

  this.name = props.name;
  this.value = props.value;
}




Preference.prototype = {
  







  Modify: function() {
    
    let weavepref = WEAVE_PREF_PREFIX + this.name;
    try {
      let syncPref = prefs.getBoolPref(weavepref);
      if (!syncPref)
        prefs.setBoolPref(weavepref, true);
    }
    catch(e) {
      Logger.AssertTrue(false, "Weave doesn't sync pref " + this.name);
    }

    
    
    let prefType = prefs.getPrefType(this.name);
    switch (prefType) {
      case CI.nsIPrefBranch.PREF_INT:
        Logger.AssertEqual(typeof(this.value), "number",
          "Wrong type used for preference value");
        prefs.setIntPref(this.name, this.value);
        break;
      case CI.nsIPrefBranch.PREF_STRING:
        Logger.AssertEqual(typeof(this.value), "string",
          "Wrong type used for preference value");
        prefs.setCharPref(this.name, this.value);
        break;
      case CI.nsIPrefBranch.PREF_BOOL:
        Logger.AssertEqual(typeof(this.value), "boolean",
          "Wrong type used for preference value");
        prefs.setBoolPref(this.name, this.value);
        break;
    }
  },

  








  Find: function() {
    
    let value;
    try {
      let prefType = prefs.getPrefType(this.name);
      switch(prefType) {
        case CI.nsIPrefBranch.PREF_INT:
          value = prefs.getIntPref(this.name);
          break;
        case CI.nsIPrefBranch.PREF_STRING:
          value = prefs.getCharPref(this.name);
          break;
        case CI.nsIPrefBranch.PREF_BOOL:
          value = prefs.getBoolPref(this.name);
          break;
      }
    }
    catch (e) {
      Logger.AssertTrue(false, "Error accessing pref " + this.name);
    }

    
    
    Logger.AssertEqual(typeof(value), typeof(this.value),
      "Value types don't match");
    Logger.AssertEqual(value, this.value, "Preference values don't match");
  },
};

