





"use strict";






const Cc = Components.classes;
const Ci = Components.interfaces;
const Cu = Components.utils;
const Cr = Components.results;

Cu.import("resource://gre/modules/XPCOMUtils.jsm");

XPCOMUtils.defineLazyModuleGetter(this, "OS",
  "resource://gre/modules/osfile.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "Promise",
  "resource://gre/modules/Promise.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "Task",
  "resource://gre/modules/Task.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "setTimeout",
  "resource://gre/modules/Timer.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "Services",
  "resource://gre/modules/Services.jsm");

function nsTerminatorTelemetry() {}

let HISTOGRAMS = {
  "quit-application": "SHUTDOWN_PHASE_DURATION_TICKS_QUIT_APPLICATION",
  "profile-change-teardown": "SHUTDOWN_PHASE_DURATION_TICKS_PROFILE_CHANGE_TEARDOWN",
  "profile-before-change":  "SHUTDOWN_PHASE_DURATION_TICKS_PROFILE_BEFORE_CHANGE",
  "xpcom-will-shutdown": "SHUTDOWN_PHASE_DURATION_TICKS_XPCOM_WILL_SHUTDOWN",
};

nsTerminatorTelemetry.prototype = {
  classID: Components.ID("{3f78ada1-cba2-442a-82dd-d5fb300ddea7}"),

  _xpcom_factory: XPCOMUtils.generateSingletonFactory(nsTerminatorTelemetry),

  
  

  QueryInterface: XPCOMUtils.generateQI([Ci.nsIObserver]),

  
  

  observe: function DS_observe(aSubject, aTopic, aData)
  {
    Task.spawn(function*() {
      
      
      
      yield new Promise(resolve => setTimeout(resolve, 3000));

      let PATH = OS.Path.join(OS.Constants.Path.localProfileDir,
        "ShutdownDuration.json");
      let raw;
      try {
        raw = yield OS.File.read(PATH, { encoding: "utf-8" });
      } catch (ex if ex.becauseNoSuchFile) {
        return;
      }
      

      
      OS.File.remove(PATH);
      OS.File.remove(PATH + ".tmp");

      let data = JSON.parse(raw);
      for (let k of Object.keys(data)) {
        let id = HISTOGRAMS[k];
        try {
          let histogram = Services.telemetry.getHistogramById(id);
          if (!histogram) {
            throw new Error("Unknown histogram " + id);
          }

          histogram.add(Number.parseInt(data[k]));
        } catch (ex) {
          
          
          Promise.reject(ex);
          continue;
        }
      }

      
      Services.obs.notifyObservers(null,
        "shutdown-terminator-telemetry-updated",
        "");
    });
  },
};




this.NSGetFactory = XPCOMUtils.generateNSGetFactory([nsTerminatorTelemetry]);
