




"use strict";

this.EXPORTED_SYMBOLS = ["AddonWatcher"];

const { classes: Cc, interfaces: Ci, utils: Cu } = Components;

Cu.import("resource://gre/modules/XPCOMUtils.jsm");

XPCOMUtils.defineLazyModuleGetter(this, "Preferences",
                                  "resource://gre/modules/Preferences.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "console",
                                  "resource://gre/modules/devtools/Console.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "PerformanceStats",
                                  "resource://gre/modules/PerformanceStats.jsm");
XPCOMUtils.defineLazyServiceGetter(this, "Telemetry",
                                  "@mozilla.org/base/telemetry;1",
                                  Ci.nsITelemetry);
XPCOMUtils.defineLazyModuleGetter(this, "Services",
                                  "resource://gre/modules/Services.jsm");

let AddonWatcher = {
  _previousPerformanceIndicators: {},
  _timer: Cc["@mozilla.org/timer;1"].createInstance(Ci.nsITimer),
  _callback: null,
  



  _interval: 15000,
  _ignoreList: null,
  
















  init: function(callback) {
    if (!callback) {
      return;
    }

    if (this._callback) {
      
      return;
    }

    this._interval = Preferences.get("browser.addon-watch.interval", 15000);
    if (this._interval == -1) {
      
      return;
    }

    this._callback = callback;
    try {
      this._ignoreList = new Set(JSON.parse(Preferences.get("browser.addon-watch.ignore", null)));
    } catch (ex) {
      
      this._ignoreList = new Set();
    }

    
    this.paused = false;

    Services.obs.addObserver(() => {
      this.uninit();
    }, "profile-before-change", false);
  },
  uninit: function() {
    this.paused = true;
    this._callback = null;
  },

  


  set paused(isPaused) {
    if (!this._callback || this._interval == -1) {
      return;
    }
    if (isPaused) {
      this._timer.cancel();
    } else {
      PerformanceStats.init();
      this._timer.initWithCallback(this._checkAddons.bind(this), this._interval, Ci.nsITimer.TYPE_REPEATING_SLACK);
    }
    this._isPaused = isPaused;
  },
  get paused() {
    return this._isPaused;
  },
  _isPaused: true,

  







  _checkAddons: function() {
    try {
      let snapshot = PerformanceStats.getSnapshot();

      let limits = {
        
        totalCPOWTime: Math.round(Preferences.get("browser.addon-watch.limits.totalCPOWTime", 1000000) * this._interval / 15000),
        
        
        longestDuration: Math.round(Math.log2(Preferences.get("browser.addon-watch.limits.longestDuration", 128))),
      };

      for (let item of snapshot.componentsData) {
        let addonId = item.addonId;
        if (!item.isSystem || !addonId) {
          
          continue;
        }
        if (this._ignoreList.has(addonId)) {
          
          
          continue;
        }
        let previous = this._previousPerformanceIndicators[addonId];
        this._previousPerformanceIndicators[addonId] = item;

        if (!previous) {
          
          
          
          
          
          continue;
        }

        

        let diff = item.substract(previous);
        if (diff.longestDuration > 5) {
          Telemetry.getKeyedHistogramById("MISBEHAVING_ADDONS_JANK_LEVEL").
            add(addonId, diff.longestDuration);
        }
        if (diff.totalCPOWTime > 0) {
          Telemetry.getKeyedHistogramById("MISBEHAVING_ADDONS_CPOW_TIME_MS").
            add(addonId, diff.totalCPOWTime);
        }

        
        let reason = null;

        for (let k of ["longestDuration", "totalCPOWTime"]) {
          if (limits[k] > 0 && diff[k] > limits[k]) {
            reason = k;
          }
        }

        if (!reason) {
          continue;
        }

        try {
          this._callback(addonId, reason);
        } catch (ex) {
          Cu.reportError("Error in AddonWatcher._checkAddons callback " + ex);
          Cu.reportError(ex.stack);
        }
      }
    } catch (ex) {
      Cu.reportError("Error in AddonWatcher._checkAddons " + ex);
      Cu.reportError(ex.stack);
    }
  },
  ignoreAddonForSession: function(addonid) {
    this._ignoreList.add(addonid);
  },
  ignoreAddonPermanently: function(addonid) {
    this._ignoreList.add(addonid);
    try {
      let ignoreList = JSON.parse(Preferences.get("browser.addon-watch.ignore", "[]"))
      if (!ignoreList.includes(addonid)) {
        ignoreList.push(addonid);
        Preferences.set("browser.addon-watch.ignore", JSON.stringify(ignoreList));
      }
    } catch (ex) {
      Preferences.set("browser.addon-watch.ignore", JSON.stringify([addonid]));
    }
  }
};
