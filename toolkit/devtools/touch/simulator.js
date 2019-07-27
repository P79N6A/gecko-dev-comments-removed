


"use strict";

let { Ci } = require("chrome");
let promise = require("promise");

const FRAME_SCRIPT =
  "resource://gre/modules/devtools/touch/simulator-content.js";

let trackedBrowsers = new WeakMap();
let savedTouchEventsEnabled =
  Services.prefs.getIntPref("dom.w3c_touch_events.enabled");





function TouchEventSimulator(browser) {
  
  let simulator = trackedBrowsers.get(browser);
  if (simulator) {
    return simulator;
  }

  let mm = browser.messageManager;
  if (!mm) {
    
    mm = browser.QueryInterface(Ci.nsIFrameLoaderOwner)
                .frameLoader.messageManager;
  }
  mm.loadFrameScript(FRAME_SCRIPT, true);

  simulator = {
    enabled: false,

    start() {
      if (this.enabled) {
        return promise.resolve({ isReloadNeeded: false });
      }
      this.enabled = true;

      let deferred = promise.defer();
      let isReloadNeeded =
        Services.prefs.getIntPref("dom.w3c_touch_events.enabled") != 1;
      Services.prefs.setIntPref("dom.w3c_touch_events.enabled", 1);
      let onStarted = () => {
        mm.removeMessageListener("TouchEventSimulator:Started", onStarted);
        deferred.resolve({ isReloadNeeded });
      };
      mm.addMessageListener("TouchEventSimulator:Started", onStarted);
      mm.sendAsyncMessage("TouchEventSimulator:Start");
      return deferred.promise;
    },

    stop() {
      if (!this.enabled) {
        return promise.resolve();
      }
      this.enabled = false;

      let deferred = promise.defer();
      Services.prefs.setIntPref("dom.w3c_touch_events.enabled",
                                savedTouchEventsEnabled);
      let onStopped = () => {
        mm.removeMessageListener("TouchEventSimulator:Stopped", onStopped);
        deferred.resolve();
      };
      mm.addMessageListener("TouchEventSimulator:Stopped", onStopped);
      mm.sendAsyncMessage("TouchEventSimulator:Stop");
      return deferred.promise;
    }
  };

  trackedBrowsers.set(browser, simulator);

  return simulator;
}

exports.TouchEventSimulator = TouchEventSimulator;
