


"use strict";

const { Cc, Ci, Cu } = require("chrome");
const Services = require("Services");
const { Class } = require("sdk/core/heritage");
loader.lazyRequireGetter(this, "events", "sdk/event/core");
loader.lazyRequireGetter(this, "EventTarget", "sdk/event/target", true);
loader.lazyRequireGetter(this, "DevToolsUtils", "devtools/toolkit/DevToolsUtils.js");
loader.lazyRequireGetter(this, "DeferredTask", "resource://gre/modules/DeferredTask.jsm", true);


const PROFILER_SYSTEM_EVENTS = [
  "console-api-profiler",
  "profiler-started",
  "profiler-stopped"
];


const BUFFER_STATUS_INTERVAL_DEFAULT = 5000; 

loader.lazyGetter(this, "nsIProfilerModule", () => {
  return Cc["@mozilla.org/tools/profiler;1"].getService(Ci.nsIProfiler);
});

let DEFAULT_PROFILER_OPTIONS = {
  
  
  entries: Math.pow(10, 7),
  
  
  interval: 1,
  features: ["js"],
  threadFilters: ["GeckoMain"]
};




const ProfilerManager = (function () {
  let consumers = new Set();

  return {

    
    _profilerStatusInterval: BUFFER_STATUS_INTERVAL_DEFAULT,

    
    _profilerStatusSubscribers: 0,

    





    addInstance: function (instance) {
      consumers.add(instance);

      
      this.registerEventListeners();
    },

    removeInstance: function (instance) {
      consumers.delete(instance);

      if (this.length < 0) {
        let msg = "Somehow the number of started profilers is now negative.";
        DevToolsUtils.reportException("Profiler", msg);
      }

      if (this.length === 0) {
        this.unregisterEventListeners();
        this.stop();
      }
    },

    










    start: function (options = {}) {
      let config = this._profilerStartOptions = {
        entries: options.entries || DEFAULT_PROFILER_OPTIONS.entries,
        interval: options.interval || DEFAULT_PROFILER_OPTIONS.interval,
        features: options.features || DEFAULT_PROFILER_OPTIONS.features,
        threadFilters: options.threadFilters || DEFAULT_PROFILER_OPTIONS.threadFilters,
      };

      
      
      let currentTime = nsIProfilerModule.getElapsedTime();

      nsIProfilerModule.StartProfiler(
        config.entries,
        config.interval,
        config.features,
        config.features.length,
        config.threadFilters,
        config.threadFilters.length
      );
      let { position, totalSize, generation } = this.getBufferInfo();

      this._updateProfilerStatusPolling();
      return { started: true, position, totalSize, generation, currentTime };
    },

    stop: function () {
      
      
      
      
      if (this.length <= 1) {
        nsIProfilerModule.StopProfiler();
      }
      this._updateProfilerStatusPolling();
      return { started: false };
    },

    

































    getProfile: function (options) {
      let startTime = options.startTime || 0;
      let profile = options.stringify ?
        nsIProfilerModule.GetProfile(startTime) :
        nsIProfilerModule.getProfileData(startTime);

      return { profile: profile, currentTime: nsIProfilerModule.getElapsedTime() };
    },

    






    getFeatures: function () {
      return { features: nsIProfilerModule.GetFeatures([]) };
    },

    






    getBufferInfo: function() {
      let position = {}, totalSize = {}, generation = {};
      nsIProfilerModule.GetBufferInfo(position, totalSize, generation);
      return {
        position: position.value,
        totalSize: totalSize.value,
        generation: generation.value
      }
    },

    





    getStartOptions: function() {
      return this._profilerStartOptions || {};
    },

    





    isActive: function() {
      let isActive = nsIProfilerModule.IsActive();
      let elapsedTime = isActive ? nsIProfilerModule.getElapsedTime() : undefined;
      let { position, totalSize, generation } = this.getBufferInfo();
      return { isActive: isActive, currentTime: elapsedTime, position, totalSize, generation };
    },

    




    getSharedLibraryInformation: function() {
      return { sharedLibraryInformation: nsIProfilerModule.getSharedLibraryInformation() };
    },

    




    get length() {
      return consumers.size;
    },

    





    observe: sanitizeHandler(function (subject, topic, data) {
      let details;

      
      
      let { action, arguments: args } = subject || {};
      let profileLabel = args && args.length > 0 ? `${args[0]}` : void 0;

      
      
      
      if (topic === "console-api-profiler" && (action === "profile" || action === "profileEnd")) {
        let { isActive, currentTime } = this.isActive();

        
        
        if (!isActive && action === "profile") {
          this.start();
          details = { profileLabel, currentTime: 0 };
        }
        
        
        else if (!isActive) {
          return;
        }

        
        
        
        details = { profileLabel, currentTime };
      }

      
      
      this.emitEvent(topic, { subject, topic, data, details });
    }, "ProfilerManager.observe"),

    









    registerEventListeners: function () {
      if (!this._eventsRegistered) {
        PROFILER_SYSTEM_EVENTS.forEach(eventName =>
          Services.obs.addObserver(this, eventName, false));
        this._eventsRegistered = true;
      }
    },

    


    unregisterEventListeners: function () {
      if (this._eventsRegistered) {
        PROFILER_SYSTEM_EVENTS.forEach(eventName => Services.obs.removeObserver(this, eventName));
        this._eventsRegistered = false;
      }
    },

    






    emitEvent: function (eventName, data) {
      let subscribers = Array.from(consumers).filter(c => c.subscribedEvents.has(eventName));

      for (let subscriber of subscribers) {
        events.emit(subscriber, eventName, data);
      }
    },

    





    setProfilerStatusInterval: function (interval) {
      this._profilerStatusInterval = interval;
      if (this._poller) {
        this._poller._delayMs = interval;
      }
    },

    subscribeToProfilerStatusEvents: function () {
      this._profilerStatusSubscribers++;
      this._updateProfilerStatusPolling();
    },

    unsubscribeToProfilerStatusEvents: function () {
      this._profilerStatusSubscribers--;
      this._updateProfilerStatusPolling();
    },

    



    _updateProfilerStatusPolling: function () {
      if (this._profilerStatusSubscribers > 0 && nsIProfilerModule.IsActive()) {
        if (!this._poller) {
          this._poller = new DeferredTask(this._emitProfilerStatus.bind(this), this._profilerStatusInterval);
        }
        this._poller.arm();
      }
      
      else if (this._poller) {
        this._poller.disarm();
      }
    },

    _emitProfilerStatus: function () {
      this.emitEvent("profiler-status", this.isActive());
      this._poller.arm();
    }
  };
})();




let Profiler = exports.Profiler = Class({
  extends: EventTarget,

  initialize: function () {
    this.subscribedEvents = new Set();
    ProfilerManager.addInstance(this);
  },

  destroy: function() {
    this.unregisterEventNotifications({ events: Array.from(this.subscribedEvents) });
    this.subscribedEvents = null;
    ProfilerManager.removeInstance(this);
  },

  


  start: function (options) { return ProfilerManager.start(options); },

  


  stop: function () { return ProfilerManager.stop(); },

  


  getProfile: function (request={}) { return ProfilerManager.getProfile(request); },

  


  getFeatures: function() { return ProfilerManager.getFeatures(); },

  


  getBufferInfo: function() { return ProfilerManager.getBufferInfo(); },

  


  getStartOptions: function() { return ProfilerManager.getStartOptions(); },

  


  isActive: function() { return ProfilerManager.isActive(); },

  


  getSharedLibraryInformation: function() { return ProfilerManager.getSharedLibraryInformation(); },

  


  setProfilerStatusInterval: function(interval) { return ProfilerManager.setProfilerStatusInterval(interval); },

  










  registerEventNotifications: function(data={}) {
    let response = [];
    (data.events || []).forEach(e => {
      if (!this.subscribedEvents.has(e)) {
        if (e === "profiler-status") {
          ProfilerManager.subscribeToProfilerStatusEvents();
        }
        this.subscribedEvents.add(e);
        response.push(e);
      }
    });
    return { registered: response };
  },

  






  unregisterEventNotifications: function(data={}) {
    let response = [];
    (data.events || []).forEach(e => {
      if (this.subscribedEvents.has(e)) {
        if (e === "profiler-status") {
          ProfilerManager.unsubscribeToProfilerStatusEvents();
        }
        this.subscribedEvents.delete(e);
        response.push(e);
      }
    });
    return { registered: response };
  },
});




function cycleBreaker(key, value) {
  if (key == "wrappedJSObject") {
    return undefined;
  }
  return value;
}













function sanitizeHandler (handler, identifier) {
  return DevToolsUtils.makeInfallible(function (subject, topic, data) {
    subject = (subject && !Cu.isXrayWrapper(subject) && subject.wrappedJSObject) || subject;
    subject = JSON.parse(JSON.stringify(subject, cycleBreaker));
    data = (data && !Cu.isXrayWrapper(data) && data.wrappedJSObject) || data;
    data = JSON.parse(JSON.stringify(data, cycleBreaker));

    
    return handler.call(this, subject, topic, data);
  }, identifier);
}
