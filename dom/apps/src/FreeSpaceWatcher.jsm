



"use strict";

const Cc = Components.classes;
const Ci = Components.interfaces;
const Cu = Components.utils;

Cu.import("resource://gre/modules/Services.jsm");

this.EXPORTED_SYMBOLS = ["FreeSpaceWatcher"];

function debug(aMsg) {
  
}


const DEFAULT_WATCHER_DELAY = 1000;

this.FreeSpaceWatcher = {
  timers: {},
  id: 0,

  











  create: function spaceWatcher_create(aThreshold, aOnStatusChange, aDelay) {
    let timer = Cc["@mozilla.org/timer;1"].createInstance(Ci.nsITimer);
    debug("Creating new FreeSpaceWatcher");
    let callback = {
      currentStatus: null,
      notify: function(aTimer) {
        try {
          let deviceStorage = Services.wm.getMostRecentWindow("navigator:browser")
                                         .navigator.getDeviceStorage("apps");
          let req = deviceStorage.freeSpace();
          req.onsuccess = req.onerror = function statResult(e) {
            if (!e.target.result) {
              return;
            }

            let freeBytes = e.target.result;
            debug("Free bytes: " + freeBytes);
            let newStatus = freeBytes > aThreshold;
            if (newStatus != callback.currentStatus) {
              debug("New status: " + (newStatus ? "free" : "full"));
              aOnStatusChange(newStatus ? "free" : "full");
              callback.currentStatus = newStatus;
            }
          }
        } catch(e) { debug(e); }
      }
    }

    timer.initWithCallback(callback, aDelay || DEFAULT_WATCHER_DELAY,
                           Ci.nsITimer.TYPE_REPEATING_SLACK);
    let id = "timer-" + this.id++;
    this.timers[id] = timer;
    return id;
  },

  



  stop: function spaceWatcher_stop(aId) {
    if (this.timers[aId]) {
      this.timers[aId].cancel();
      delete this.timers[aId];
    }
  }
}
