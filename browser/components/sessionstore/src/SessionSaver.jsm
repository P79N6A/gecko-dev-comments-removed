



"use strict";

this.EXPORTED_SYMBOLS = ["SessionSaver"];

const Cu = Components.utils;
const Cc = Components.classes;
const Ci = Components.interfaces;

Cu.import("resource://gre/modules/Timer.jsm", this);
Cu.import("resource://gre/modules/Services.jsm", this);
Cu.import("resource://gre/modules/XPCOMUtils.jsm", this);
Cu.import("resource://gre/modules/TelemetryStopwatch.jsm", this);

XPCOMUtils.defineLazyModuleGetter(this, "SessionStore",
  "resource:///modules/sessionstore/SessionStore.jsm");

XPCOMUtils.defineLazyModuleGetter(this, "_SessionFile",
  "resource:///modules/sessionstore/_SessionFile.jsm");


XPCOMUtils.defineLazyGetter(this, "gInterval", function () {
  const PREF = "browser.sessionstore.interval";

  
  Services.prefs.addObserver(PREF, () => {
    this.gInterval = Services.prefs.getIntPref(PREF);

    
    
    SessionSaverInternal.cancel();
    SessionSaverInternal.runDelayed(0);
  }, false);

  return Services.prefs.getIntPref(PREF);
});


function createSupportsString(data) {
  let string = Cc["@mozilla.org/supports-string;1"]
                 .createInstance(Ci.nsISupportsString);
  string.data = data;
  return string;
}


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
    SessionSaverInternal.run();
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
    this._saveState(true );
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

    stopWatchStart("COLLECT_DATA_MS", "COLLECT_DATA_LONGEST_OP_MS");

    let state = SessionStore.getCurrentState(forceUpdateAllWindows);
    if (!state) {
      stopWatchCancel("COLLECT_DATA_MS", "COLLECT_DATA_LONGEST_OP_MS");
      return;
    }

    
    for (let i = state.windows.length - 1; i >= 0; i--) {
      if (state.windows[i].isPrivate) {
        state.windows.splice(i, 1);
        if (state.selectedWindow >= i) {
          state.selectedWindow--;
        }
      }
    }

#ifndef XP_MACOSX
    
    
    if (state.windows.length == 0) {
      stopWatchCancel("COLLECT_DATA_MS", "COLLECT_DATA_LONGEST_OP_MS");
      return;
    }
#endif

    
    for (let i = state._closedWindows.length - 1; i >= 0; i--) {
      if (state._closedWindows[i].isPrivate) {
        state._closedWindows.splice(i, 1);
      }
    }

#ifndef XP_MACOSX
    
    
    
    while (state._closedWindows.length) {
      let i = state._closedWindows.length - 1;

      if (!state._closedWindows[i]._shouldRestore) {
        
        
        break;
      }

      delete state._closedWindows[i]._shouldRestore;
      state.windows.unshift(state._closedWindows.pop());
    }
#endif

    stopWatchFinish("COLLECT_DATA_MS", "COLLECT_DATA_LONGEST_OP_MS");
    this._writeState(state);
  },

  




  _saveStateAsync: function () {
    
    this._timeoutID = null;

    
    if (!Services.prefs.getBoolPref("browser.sessionstore.async")) {
      this._saveState();
      return;
    }

    
    
    this.updateLastSaveTime();

    
    
    
    
    
    SessionStore.fillTabCachesAsynchronously().then(() => this._saveState());
  },

  


  _writeState: function (state) {
    stopWatchStart("SERIALIZE_DATA_MS", "SERIALIZE_DATA_LONGEST_OP_MS");
    let data = JSON.stringify(state);
    stopWatchFinish("SERIALIZE_DATA_MS", "SERIALIZE_DATA_LONGEST_OP_MS");

    
    data = this._notifyObserversBeforeStateWrite(data);

    
    if (!data) {
      return;
    }

    
    
    
    
    this.updateLastSaveTime();

    
    
    
    _SessionFile.write(data).then(() => {
      this.updateLastSaveTime();
      notify(null, "sessionstore-state-write-complete");
    }, Cu.reportError);
  },

  



  _notifyObserversBeforeStateWrite: function (data) {
    let stateString = createSupportsString(data);
    notify(stateString, "sessionstore-state-write");
    return stateString.data;
  }
};
