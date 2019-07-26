



"use strict";

this.EXPORTED_SYMBOLS = ["TabStateCache"];

const Cu = Components.utils;
Cu.import("resource://gre/modules/Services.jsm", this);












this.TabStateCache = Object.freeze({
  





  has: function (aTab) {
    return TabStateCacheInternal.has(aTab);
  },

  







  set: function(aTab, aValue) {
    return TabStateCacheInternal.set(aTab, aValue);
  },

  







  get: function(aKey) {
    return TabStateCacheInternal.get(aKey);
  },

  






  delete: function(aKey) {
    return TabStateCacheInternal.delete(aKey);
  },

  


  clear: function() {
    return TabStateCacheInternal.clear();
  },

  







  updateField: function(aKey, aField, aValue) {
    return TabStateCacheInternal.updateField(aKey, aField, aValue);
  },

  






  removeField: function(aKey, aField) {
    return TabStateCacheInternal.removeField(aKey, aField);
  },

  


  get hits() {
    return TabStateCacheTelemetry.hits;
  },

  


  get misses() {
    return TabStateCacheTelemetry.misses;
  },

  


  get clears() {
    return TabStateCacheTelemetry.clears;
  },
});

let TabStateCacheInternal = {
  _data: new WeakMap(),

  





  has: function (aTab) {
    let key = this._normalizeToBrowser(aTab);
    return this._data.has(key);
  },

  







  set: function(aTab, aValue) {
    let key = this._normalizeToBrowser(aTab);
    this._data.set(key, aValue);
  },

  







  get: function(aKey) {
    let key = this._normalizeToBrowser(aKey);
    let result = this._data.get(key);
    TabStateCacheTelemetry.recordAccess(!!result);
    return result;
  },

  






  delete: function(aKey) {
    let key = this._normalizeToBrowser(aKey);
    this._data.delete(key);
  },

  


  clear: function() {
    TabStateCacheTelemetry.recordClear();
    this._data.clear();
  },

  







  updateField: function(aKey, aField, aValue) {
    let key = this._normalizeToBrowser(aKey);
    let data = this._data.get(key);
    if (data) {
      data[aField] = aValue;
    }
    TabStateCacheTelemetry.recordAccess(!!data);
  },

  






  removeField: function(aKey, aField) {
    let key = this._normalizeToBrowser(aKey);
    let data = this._data.get(key);
    if (data && aField in data) {
      delete data[aField];
    }
    TabStateCacheTelemetry.recordAccess(!!data);
  },

  _normalizeToBrowser: function(aKey) {
    let nodeName = aKey.localName;
    if (nodeName == "tab") {
      return aKey.linkedBrowser;
    }
    if (nodeName == "browser") {
      return aKey;
    }
    throw new TypeError("Key is neither a tab nor a browser: " + nodeName);
  }
};

let TabStateCacheTelemetry = {
  
  hits: 0,
  
  misses: 0,
  
  clears: 0,
  
  _initialized: false,

  





  recordAccess: function(isHit) {
    this._init();
    if (isHit) {
      ++this.hits;
    } else {
      ++this.misses;
    }
  },

  


  recordClear: function() {
    this._init();
    ++this.clears;
  },

  


  _init: function() {
    if (this._initialized) {
      
      return;
    }
    this._initialized = true;
    Services.obs.addObserver(this, "profile-before-change", false);
  },

  observe: function() {
    Services.obs.removeObserver(this, "profile-before-change");

    
    let accesses = this.hits + this.misses;
    if (accesses == 0) {
      return;
    }

    this._fillHistogram("HIT_RATE", this.hits, accesses);
    this._fillHistogram("CLEAR_RATIO", this.clears, accesses);
  },

  _fillHistogram: function(suffix, positive, total) {
    let PREFIX = "FX_SESSION_RESTORE_TABSTATECACHE_";
    let histo = Services.telemetry.getHistogramById(PREFIX + suffix);
    let rate = Math.floor( ( positive * 100 ) / total );
    histo.add(rate);
  }
};
