




"use strict";

this.EXPORTED_SYMBOLS = ["SharedPreferences"];

const { classes: Cc, interfaces: Ci, utils: Cu } = Components;


Cu.import("resource://gre/modules/Services.jsm");
Cu.import("resource://gre/modules/Messaging.jsm");

let Scope = Object.freeze({
  APP:          "app",
  PROFILE:      "profile",
  GLOBAL:       "global"
});




let SharedPreferences = {
  forApp: function() {
    return new SharedPreferencesImpl({ scope: Scope.APP });
  },

  forProfile: function() {
    return new SharedPreferencesImpl({ scope: Scope.PROFILE });
  },

  



  forProfileName: function(profileName) {
    return new SharedPreferencesImpl({ scope: Scope.PROFILE, profileName: profileName });
  },

  




  forAndroid: function(branch) {
    return new SharedPreferencesImpl({ scope: Scope.GLOBAL, branch: branch });
  }
};











function SharedPreferencesImpl(options = {}) {
  if (!(this instanceof SharedPreferencesImpl)) {
    return new SharedPreferencesImpl(level);
  }

  if (options.scope == null || options.scope == undefined) {
    throw "Shared Preferences must specifiy a scope.";
  }

  this._scope = options.scope;
  this._profileName = options.profileName;
  this._branch = options.branch;
  this._observers = {};
}

SharedPreferencesImpl.prototype = Object.freeze({
  _set: function _set(prefs) {
    Messaging.sendRequest({
      type: "SharedPreferences:Set",
      preferences: prefs,
      scope: this._scope,
      profileName: this._profileName,
      branch: this._branch,
    });
  },

  _setOne: function _setOne(prefName, value, type) {
    let prefs = [];
    prefs.push({
      name: prefName,
      value: value,
      type: type,
    });
    this._set(prefs);
  },

  setBoolPref: function setBoolPref(prefName, value) {
    this._setOne(prefName, value, "bool");
  },

  setCharPref: function setCharPref(prefName, value) {
    this._setOne(prefName, value, "string");
  },

  setIntPref: function setIntPref(prefName, value) {
    this._setOne(prefName, value, "int");
  },

  _get: function _get(prefs, callback) {
    let result = null;
    Messaging.sendRequestForResult({
      type: "SharedPreferences:Get",
      preferences: prefs,
      scope: this._scope,
      profileName: this._profileName,
      branch: this._branch,
    }).then((data) => {
      result = data.values;
    });

    let thread = Services.tm.currentThread;
    while (result == null)
      thread.processNextEvent(true);

    return result;
  },

  _getOne: function _getOne(prefName, type) {
    let prefs = [];
    prefs.push({
      name: prefName,
      type: type,
    });
    let values = this._get(prefs);
    if (values.length != 1) {
      throw new Error("Got too many values: " + values.length);
    }
    return values[0].value;
  },

  getBoolPref: function getBoolPref(prefName) {
    return this._getOne(prefName, "bool");
  },

  getCharPref: function getCharPref(prefName) {
    return this._getOne(prefName, "string");
  },

  getIntPref: function getIntPref(prefName) {
    return this._getOne(prefName, "int");
  },

  





  addObserver: function addObserver(domain, observer, holdWeak) {
    if (!domain)
      throw new Error("domain must not be null");
    if (!observer)
      throw new Error("observer must not be null");
    if (holdWeak)
      throw new Error("Weak references not yet implemented.");

    if (!this._observers.hasOwnProperty(domain))
      this._observers[domain] = [];
    if (this._observers[domain].indexOf(observer) > -1)
      return;

    this._observers[domain].push(observer);

    this._updateAndroidListener();
  },

  



  removeObserver: function removeObserver(domain, observer) {
    if (!this._observers.hasOwnProperty(domain))
      return;
    let index = this._observers[domain].indexOf(observer);
    if (index < 0)
      return;

    this._observers[domain].splice(index, 1);
    if (this._observers[domain].length < 1)
      delete this._observers[domain];

    this._updateAndroidListener();
  },

  _updateAndroidListener: function _updateAndroidListener() {
    if (this._listening && Object.keys(this._observers).length < 1)
      this._uninstallAndroidListener();
    if (!this._listening && Object.keys(this._observers).length > 0)
      this._installAndroidListener();
  },

  _installAndroidListener: function _installAndroidListener() {
    if (this._listening)
      return;
    this._listening = true;

    Services.obs.addObserver(this, "SharedPreferences:Changed", false);
    Messaging.sendRequest({
      type: "SharedPreferences:Observe",
      enable: true,
      scope: this._scope,
      profileName: this._profileName,
      branch: this._branch,
    });
  },

  observe: function observe(subject, topic, data) {
    if (topic != "SharedPreferences:Changed") {
      return;
    }

    let msg = JSON.parse(data);
    if (msg.scope !== this._scope ||
        ((this._scope === Scope.PROFILE) && (msg.profileName !== this._profileName)) ||
        ((this._scope === Scope.GLOBAL)  && (msg.branch !== this._branch))) {
      return;
    }

    if (!this._observers.hasOwnProperty(msg.key)) {
      return;
    }

    let observers = this._observers[msg.key];
    for (let obs of observers) {
      obs.observe(obs, msg.key, msg.value);
    }
  },

  _uninstallAndroidListener: function _uninstallAndroidListener() {
    if (!this._listening)
      return;
    this._listening = false;

    Services.obs.removeObserver(this, "SharedPreferences:Changed");
    Messaging.sendRequest({
      type: "SharedPreferences:Observe",
      enable: false,
      scope: this._scope,
      profileName: this._profileName,
      branch: this._branch,
    });
  },
});
