



"use strict";

this.EXPORTED_SYMBOLS = ["SessionSaver"];

const Cu = Components.utils;
const Cc = Components.classes;
const Ci = Components.interfaces;

Cu.import("resource://gre/modules/Timer.jsm", this);
Cu.import("resource://gre/modules/Services.jsm", this);
Cu.import("resource://gre/modules/XPCOMUtils.jsm", this);
Cu.import("resource://gre/modules/TelemetryStopwatch.jsm", this);

XPCOMUtils.defineLazyModuleGetter(this, "console",
  "resource://gre/modules/devtools/Console.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "PrivacyFilter",
  "resource:///modules/sessionstore/PrivacyFilter.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "RevivableWindows",
  "resource:///modules/sessionstore/RevivableWindows.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "SessionStore",
  "resource:///modules/sessionstore/SessionStore.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "SessionFile",
  "resource:///modules/sessionstore/SessionFile.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "PrivateBrowsingUtils",
  "resource://gre/modules/PrivateBrowsingUtils.jsm");


XPCOMUtils.defineLazyGetter(this, "gInterval", function () {
  const PREF = "browser.sessionstore.interval";

  
  Services.prefs.addObserver(PREF, () => {
    this.gInterval = Services.prefs.getIntPref(PREF);

    
    
    SessionSaverInternal.cancel();
    SessionSaverInternal.runDelayed(0);
  }, false);

  return Services.prefs.getIntPref(PREF);
});


function notify(subject, topic) {
  Services.obs.notifyObservers(subject, topic, "");
}


function stopWatch(method) {
  return function (...histograms) {
    for (let hist of histograms) {
      TelemetryStopwatch[method]("FX_SESSION_RESTORE_" + hist);
    }
  };
}

let stopWatchStart = stopWatch("start");
let stopWatchCancel = stopWatch("cancel");
let stopWatchFinish = stopWatch("finish");




this.SessionSaver = Object.freeze({
  


  run: function () {
    return SessionSaverInternal.run();
  },

  




  runDelayed: function () {
    SessionSaverInternal.runDelayed();
  },

  



  updateLastSaveTime: function () {
    SessionSaverInternal.updateLastSaveTime();
  },

  



  clearLastSaveTime: function () {
    SessionSaverInternal.clearLastSaveTime();
  },

  


  cancel: function () {
    SessionSaverInternal.cancel();
  }
});




let SessionSaverInternal = {
  



  _timeoutID: null,

  




  _lastSaveTime: 0,

  


  run: function () {
    return this._saveState(true );
  },

  








  runDelayed: function (delay = 2000) {
    
    if (this._timeoutID) {
      return;
    }

    
    delay = Math.max(this._lastSaveTime + gInterval - Date.now(), delay, 0);

    
    this._timeoutID = setTimeout(() => this._saveStateAsync(), delay);
  },

  



  updateLastSaveTime: function () {
    this._lastSaveTime = Date.now();
  },

  



  clearLastSaveTime: function () {
    this._lastSaveTime = 0;
  },

  


  cancel: function () {
    clearTimeout(this._timeoutID);
    this._timeoutID = null;
  },

  






  _saveState: function (forceUpdateAllWindows = false) {
    
    this.cancel();

    if (PrivateBrowsingUtils.permanentPrivateBrowsing) {
      
      

      this.updateLastSaveTime();
      return Promise.resolve();
    }

    stopWatchStart("COLLECT_DATA_MS", "COLLECT_DATA_LONGEST_OP_MS");
    let state = SessionStore.getCurrentState(forceUpdateAllWindows);
    PrivacyFilter.filterPrivateWindowsAndTabs(state);

    
    
    if (state.deferredInitialState) {
      state.windows = state.deferredInitialState.windows || [];
      delete state.deferredInitialState;
    }

    
    
    
    
    let windowsToRevive = RevivableWindows.get();
    state.windows.unshift(...windowsToRevive);
    let revivedWindows = state._closedWindows.splice(0, windowsToRevive.length);
#ifdef DEBUG
    
    
    let match = revivedWindows.every((win, idx) => {
      return win == windowsToRevive[windowsToRevive.length - 1 - idx];
    });

    if (!match) {
      throw new Error("SessionStore: revived windows didn't match closed windows");
    }
#endif DEBUG

    stopWatchFinish("COLLECT_DATA_MS", "COLLECT_DATA_LONGEST_OP_MS");
    return this._writeState(state);
  },

  




  _saveStateAsync: function () {
    
    this._timeoutID = null;

    
    this._saveState();
  },

  


  _writeState: function (state) {
    
    notify(null, "sessionstore-state-write");

    stopWatchStart("SERIALIZE_DATA_MS", "SERIALIZE_DATA_LONGEST_OP_MS", "WRITE_STATE_LONGEST_OP_MS");
    let data = JSON.stringify(state);
    stopWatchFinish("SERIALIZE_DATA_MS", "SERIALIZE_DATA_LONGEST_OP_MS");

    
    
    
    
    this.updateLastSaveTime();

    
    
    
    stopWatchStart("SEND_SERIALIZED_STATE_LONGEST_OP_MS");
    let promise = SessionFile.write(data);
    stopWatchFinish("WRITE_STATE_LONGEST_OP_MS",
                    "SEND_SERIALIZED_STATE_LONGEST_OP_MS");
    promise = promise.then(() => {
      this.updateLastSaveTime();
      notify(null, "sessionstore-state-write-complete");
    }, console.error);

    return promise;
  },
};
