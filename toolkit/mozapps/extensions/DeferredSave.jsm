



"use strict";

const Cu = Components.utils;
const Cc = Components.classes;
const Ci = Components.interfaces;

Cu.import("resource://gre/modules/osfile.jsm");
Cu.import("resource://gre/modules/Promise.jsm");

this.EXPORTED_SYMBOLS = ["DeferredSave"];


const DEFAULT_SAVE_DELAY_MS = 50;























function DeferredSave(aPath, aDataProvider, aDelay) {
  
  let leafName = OS.Path.basename(aPath);
  Cu.import("resource://gre/modules/AddonLogging.jsm");
  LogManager.getLogger("DeferredSave/" + leafName, this);

  
  
  
  
  this._pending = null;

  
  
  
  
  
  
  
  
  
  
  this._writing = Promise.resolve(0);

  
  this.writeInProgress = false;

  this._path = aPath;
  this._dataProvider = aDataProvider;

  this._timer = null;

  
  
  this.totalSaves = 0;
  
  
  this.overlappedSaves = 0;

  if (aDelay && (aDelay > 0))
    this._delay = aDelay;
  else
    this._delay = DEFAULT_SAVE_DELAY_MS;
}

DeferredSave.prototype = {
  get dirty() {
    return this._pending || this.writeInProgress;
  },

  get error() {
    return this._lastError;
  },

  
  _startTimer: function() {
    if (!this._pending) {
      return;
    }

    this.LOG("Starting timer");
    if (!this._timer)
      this._timer = Cc["@mozilla.org/timer;1"].createInstance(Ci.nsITimer);
    this._timer.initWithCallback(() => this._deferredSave(),
                                 this._delay, Ci.nsITimer.TYPE_ONE_SHOT);
  },

  




  saveChanges: function() {
    this.LOG("Save changes");
    if (!this._pending) {
      if (this.writeInProgress) {
        this.LOG("Data changed while write in progress");
        this.overlappedSaves++;
      }
      this._pending = Promise.defer();
      
      
      this._writing.then(count => this._startTimer(), error => this._startTimer());
    }
    return this._pending.promise;
  },

  _deferredSave: function() {
    let pending = this._pending;
    this._pending = null;
    let writing = this._writing;
    this._writing = pending.promise;

    
    
    let toSave = null;
    try {
      toSave = this._dataProvider();
    }
    catch(e) {
      this.ERROR("Deferred save dataProvider failed", e);
      writing.then(null, error => {})
        .then(count => {
          pending.reject(e);
        });
      return;
    }

    writing.then(null, error => {return 0;})
    .then(count => {
      this.LOG("Starting write");
      this.totalSaves++;
      this.writeInProgress = true;

      OS.File.writeAtomic(this._path, toSave, {tmpPath: this._path + ".tmp"})
      .then(
        result => {
          this._lastError = null;
          this.writeInProgress = false;
          this.LOG("Write succeeded");
          pending.resolve(result);
        },
        error => {
          this._lastError = error;
          this.writeInProgress = false;
          this.WARN("Write failed", error);
          pending.reject(error);
        });
    });
  },

  
















  flush: function() {
    
    
    
    if (this._pending) {
      this.LOG("Flush called while data is dirty");
      if (this._timer) {
        this._timer.cancel();
        this._timer = null;
      }
      this._deferredSave();
    }

    return this._writing;
  }
}
