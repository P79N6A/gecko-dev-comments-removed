






































Components.utils.import("resource://gre/modules/XPCOMUtils.jsm");
Components.utils.import("resource://gre/modules/Services.jsm");

const Cc = Components.classes;
const Ci = Components.interfaces;

const PREF_APP_UPDATE_LASTUPDATETIME_FMT  = "app.update.lastUpdateTime.%ID%";
const PREF_APP_UPDATE_TIMER               = "app.update.timer";
const PREF_APP_UPDATE_TIMERFIRSTINTERVAL  = "app.update.timerFirstInterval";
const PREF_APP_UPDATE_LOG                 = "app.update.log";

const CATEGORY_UPDATE_TIMER               = "update-timer";

XPCOMUtils.defineLazyGetter(this, "gLogEnabled", function tm_gLogEnabled() {
  return getPref("getBoolPref", PREF_APP_UPDATE_LOG, false);
});













function getPref(func, preference, defaultValue) {
  try {
    return Services.prefs[func](preference);
  }
  catch (e) {
  }
  return defaultValue;
}






function LOG(string) {
  if (gLogEnabled) {
    dump("*** UTM:SVC " + string + "\n");
    Services.console.logStringMessage("UTM:SVC " + string);
  }
}






function TimerManager() {
  Services.obs.addObserver(this, "xpcom-shutdown", false);
}
TimerManager.prototype = {
  


  _timer: null,

  



   _timerInterval: null,

  


  _timers: { },

  







  get _fudge() {
    return Math.round(Math.random() * this._timerInterval / 1000);
  },

  


  observe: function TM_observe(aSubject, aTopic, aData) {
    
    var minInterval = 60000;
    
    
    var minFirstInterval = 10000;
    switch (aTopic) {
    case "utm-test-init":
      
      
      minInterval = 500;
      minFirstInterval = 500;
    case "profile-after-change":
      
      
      if (this._timer) {
        this._timer.cancel();
        this._timer = null;
      }
      this._timerInterval = Math.max(getPref("getIntPref", PREF_APP_UPDATE_TIMER, 600000),
                                     minInterval);
      let firstInterval = Math.max(getPref("getIntPref", PREF_APP_UPDATE_TIMERFIRSTINTERVAL,
                                           this._timerInterval), minFirstInterval);
      this._timer = Cc["@mozilla.org/timer;1"].createInstance(Ci.nsITimer);
      this._timer.initWithCallback(this, firstInterval,
                                   Ci.nsITimer.TYPE_REPEATING_SLACK);
      break;
    case "xpcom-shutdown":
      Services.obs.removeObserver(this, "xpcom-shutdown");

      
      if (this._timer) {
        this._timer.cancel();
        this._timer = null;
      }
      for (var timerID in this._timers)
        delete this._timers[timerID];
      this._timers = null;
      break;
    }
  },

  




  notify: function TM_notify(timer) {
    if (timer.delay != this._timerInterval)
      timer.delay = this._timerInterval;

    var prefLastUpdate;
    var lastUpdateTime;
    var now = Math.round(Date.now() / 1000);
    var catMan = Cc["@mozilla.org/categorymanager;1"].
                 getService(Ci.nsICategoryManager);
    var entries = catMan.enumerateCategory(CATEGORY_UPDATE_TIMER);
    while (entries.hasMoreElements()) {
      let entry = entries.getNext().QueryInterface(Ci.nsISupportsCString).data;
      let value = catMan.getCategoryEntry(CATEGORY_UPDATE_TIMER, entry);
      let [cid, method, timerID, prefInterval, defaultInterval] = value.split(",");
      defaultInterval = parseInt(defaultInterval);
      
      if (!timerID || !defaultInterval || isNaN(defaultInterval)) {
        LOG("TimerManager:notify - update-timer category registered" +
            (cid ? " for " + cid : "") + " without required parameters - " +
             "skipping");
        continue;
      }

      let interval = getPref("getIntPref", prefInterval, defaultInterval);
      prefLastUpdate = PREF_APP_UPDATE_LASTUPDATETIME_FMT.replace(/%ID%/,
                                                                  timerID);
      if (Services.prefs.prefHasUserValue(prefLastUpdate)) {
        lastUpdateTime = Services.prefs.getIntPref(prefLastUpdate);
      }
      else {
        lastUpdateTime = now + this._fudge;
        Services.prefs.setIntPref(prefLastUpdate, lastUpdateTime);
        continue;
      }

      if ((now - lastUpdateTime) > interval) {
        try {
          Components.classes[cid][method](Ci.nsITimerCallback).notify(timer);
          LOG("TimerManager:notify - notified " + cid);
        }
        catch (e) {
          LOG("TimerManager:notify - error notifying component id: " +
              cid + " ,error: " + e);
        }
        lastUpdateTime = now + this._fudge;
        Services.prefs.setIntPref(prefLastUpdate, lastUpdateTime);
      }
    }

    for (var timerID in this._timers) {
      var timerData = this._timers[timerID];

      if ((now - timerData.lastUpdateTime) > timerData.interval) {
        if (timerData.callback instanceof Ci.nsITimerCallback) {
          try {
            timerData.callback.notify(timer);
            LOG("TimerManager:notify - notified timerID: " + timerID);
          }
          catch (e) {
            LOG("TimerManager:notify - error notifying timerID: " + timerID +
                ", error: " + e);
          }
        }
        else {
          LOG("TimerManager:notify - timerID: " + timerID + " doesn't " +
              "implement nsITimerCallback - skipping");
        }
        lastUpdateTime = now + this._fudge;
        timerData.lastUpdateTime = lastUpdateTime;
        prefLastUpdate = PREF_APP_UPDATE_LASTUPDATETIME_FMT.replace(/%ID%/, timerID);
        Services.prefs.setIntPref(prefLastUpdate, lastUpdateTime);
      }
    }
  },

  


  registerTimer: function TM_registerTimer(id, callback, interval) {
    LOG("TimerManager:registerTimer - id: " + id);
    var prefLastUpdate = PREF_APP_UPDATE_LASTUPDATETIME_FMT.replace(/%ID%/, id);
    var lastUpdateTime;
    if (Services.prefs.prefHasUserValue(prefLastUpdate)) {
      lastUpdateTime = Services.prefs.getIntPref(prefLastUpdate);
    } else {
      lastUpdateTime = Math.round(Date.now() / 1000) + this._fudge;
      Services.prefs.setIntPref(prefLastUpdate, lastUpdateTime);
    }
    this._timers[id] = { callback       : callback,
                         interval       : interval,
                         lastUpdateTime : lastUpdateTime };
  },

  classID: Components.ID("{B322A5C0-A419-484E-96BA-D7182163899F}"),
  QueryInterface: XPCOMUtils.generateQI([Ci.nsIUpdateTimerManager,
                                         Ci.nsITimerCallback,
                                         Ci.nsIObserver])
};

var NSGetFactory = XPCOMUtils.generateNSGetFactory([TimerManager]);
