



"use strict";

this.EXPORTED_SYMBOLS = [
  "TelemetryEnvironment",
];

const {classes: Cc, interfaces: Ci, utils: Cu} = Components;

Cu.import("resource://gre/modules/XPCOMUtils.jsm", this);
Cu.import("resource://gre/modules/Task.jsm");
Cu.import("resource://gre/modules/Log.jsm");
Cu.import("resource://gre/modules/Preferences.jsm");

const LOGGER_NAME = "Toolkit.Telemetry";

this.TelemetryEnvironment = {
  _shutdown: true,

  
  _changeListeners: new Map(),
  
  _collectTask: null,
  _doNotify: false,

  
  RECORD_PREF_STATE: 1, 
  RECORD_PREF_VALUE: 2, 

  
  
  _watchedPrefs: null,

  


  init: function () {
    if (!this._shutdown) {
      this._log.error("init - Already initialized");
      return;
    }

    this._log = Log.repository.getLoggerWithMessagePrefix(LOGGER_NAME, "TelemetryEnvironment::");
    this._log.trace("init");
    this._shutdown = false;
    this._startWatchingPrefs();
  },

  



  shutdown: Task.async(function* () {
    if (this._shutdown) {
      this._log.error("shutdown - Already shut down");
      throw new Error("Already shut down");
    }

    this._log.trace("shutdown");
    this._shutdown = true;
    this._stopWatchingPrefs();
    this._changeListeners.clear();
    yield this._collectTask;
  }),

  




  registerChangeListener: function (name, listener) {
    this._log.trace("registerChangeListener for " + name);
    if (this._shutdown) {
      this._log.warn("registerChangeListener - already shutdown")
      return;
    }
    this._changeListeners.set(name, listener);
  },

  



  unregisterChangeListener: function (name) {
    this._log.trace("unregisterChangeListener for " + name);
    if (this._shutdown) {
      this._log.warn("registerChangeListener - already shutdown")
      return;
    }
    this._changeListeners.delete(name);
  },

  



  _watchPreferences: function (aPreferences) {
    if (this._watchedPrefs) {
      this._stopWatchingPrefs();
    }

    this._watchedPrefs = aPreferences;
    this._startWatchingPrefs();
  },

  





  _getPrefData: function () {
    if (!this._watchedPrefs) {
      return {};
    }

    let prefData = {};
    for (let pref in this._watchedPrefs) {
      
      if (!Preferences.isSet(pref)) {
        continue;
      }

      
      
      let prefValue = undefined;
      if (this._watchedPrefs[pref] == this.RECORD_PREF_STATE) {
        prefValue = null;
      } else {
        prefValue = Preferences.get(pref, null);
      }
      prefData[pref] = prefValue;
    }
    return prefData;
  },

  


  _startWatchingPrefs: function () {
    this._log.trace("_startWatchingPrefs - " + this._watchedPrefs);

    if (!this._watchedPrefs) {
      return;
    }

    for (let pref in this._watchedPrefs) {
      Preferences.observe(pref, this._onPrefChanged, this);
    }
  },

  


  _stopWatchingPrefs: function () {
    this._log.trace("_stopWatchingPrefs");

    if (!this._watchedPrefs) {
      return;
    }

    for (let pref in this._watchedPrefs) {
      Preferences.ignore(pref, this._onPrefChanged, this);
    }

    this._watchedPrefs = null;
  },

  _onPrefChanged: function () {
    this._log.trace("_onPrefChanged");
    this._onEnvironmentChange("pref-changed");
  },

  



  _getSettings: function () {
    return {
      "userPrefs": this._getPrefData(),
    };
  },

  



  getEnvironmentData: function() {
    if (this._shutdown) {
      this._log.error("getEnvironmentData - Already shut down");
      return Promise.reject("Already shutdown");
    }

    this._log.trace("getEnvironmentData");
    if (this._collectTask) {
      return this._collectTask;
    }

    this._collectTask = this._doGetEnvironmentData();
    let clear = () => this._collectTask = null;
    this._collectTask.then(clear, clear);
    return this._collectTask;
  },

  _doGetEnvironmentData: Task.async(function* () {
    this._log.trace("getEnvironmentData");

    
    let sections = {
      "settings": () => this._getSettings(),
    };

    let data = {};
    
    
    
    for (let s in sections) {
      try {
        data[s] = sections[s]();
      } catch (e) {
        this._log.error("getEnvironmentData - There was an exception collecting " + s, e);
      }
    }

    return data;
  }),

  _onEnvironmentChange: function (what) {
    this._log.trace("_onEnvironmentChange for " + what);
    if (this._shutdown) {
      this._log.trace("_onEnvironmentChange - Already shut down.");
      return;
    }

    for (let [name, listener] of this._changeListeners) {
      try {
        this._log.debug("_onEnvironmentChange - calling " + name);
        listener();
      } catch (e) {
        this._log.warning("_onEnvironmentChange - listener " + name + " caught error", e);
      }
    }
  },
};
