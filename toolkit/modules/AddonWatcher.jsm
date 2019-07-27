




"use strict";

this.EXPORTED_SYMBOLS = ["AddonWatcher"];

const { classes: Cc, interfaces: Ci, utils: Cu } = Components;

Cu.import("resource://gre/modules/XPCOMUtils.jsm");

XPCOMUtils.defineLazyModuleGetter(this, "Preferences",
                                  "resource://gre/modules/Preferences.jsm");

let AddonWatcher = {
  _lastAddonTime: {},
  _timer: Cc["@mozilla.org/timer;1"].createInstance(Ci.nsITimer),
  _callback: null,
  _interval: 1500,
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
    this._timer.initWithCallback(this._checkAddons.bind(this), this._interval, Ci.nsITimer.TYPE_REPEATING_SLACK);
  },
  uninit: function() {
    if (this._timer) {
      this._timer.cancel();
      this._timer = null;
    }
  },
  _checkAddons: function() {
    let compartmentInfo = Cc["@mozilla.org/compartment-info;1"]
      .getService(Ci.nsICompartmentInfo);
    let compartments = compartmentInfo.getCompartments();
    let count = compartments.length;
    let addons = {};
    for (let i = 0; i < count; i++) {
      let compartment = compartments.queryElementAt(i, Ci.nsICompartment);
      if (compartment.addonId) {
        if (addons[compartment.addonId]) {
          addons[compartment.addonId] += compartment.time;
        } else {
          addons[compartment.addonId] = compartment.time;
        }
      }
    }
    let limit = this._interval * Preferences.get("browser.addon-watch.percentage-limit", 75) * 10;
    for (let addonId in addons) {
      if (!this._ignoreList.has(addonId)) {
        if (!this._lastAddonTime[addonId]) {
          this._lastAddonTime[addonId] = 0;
        }
        if ((addons[addonId] - this._lastAddonTime[addonId]) > limit) {
          this._callback(addonId);
        }
        this._lastAddonTime[addonId] = addons[addonId];
      }
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
