


"use strict";

const {Cc, Ci, Cu, Cr} = require("chrome");
const Services = require("Services");
const DevToolsUtils = require("devtools/toolkit/DevToolsUtils.js");

let DEFAULT_PROFILER_ENTRIES = 10000000;
let DEFAULT_PROFILER_INTERVAL = 1;
let DEFAULT_PROFILER_FEATURES = ["js"];
let DEFAULT_PROFILER_THREADFILTERS = ["GeckoMain"];






let gProfilerConsumers = 0;

loader.lazyGetter(this, "nsIProfilerModule", () => {
  return Cc["@mozilla.org/tools/profiler;1"].getService(Ci.nsIProfiler);
});




function ProfilerActor() {
  gProfilerConsumers++;
  this._observedEvents = new Set();
}

ProfilerActor.prototype = {
  actorPrefix: "profiler",
  disconnect: function() {
    for (let event of this._observedEvents) {
      Services.obs.removeObserver(this, event);
    }
    this._observedEvents = null;
    this.onStopProfiler();

    gProfilerConsumers--;
    checkProfilerConsumers();
  },

  




  onGetFeatures: function() {
    return { features: nsIProfilerModule.GetFeatures([]) };
  },

  








  onStartProfiler: function(request = {}) {
    nsIProfilerModule.StartProfiler(
      (request.entries || DEFAULT_PROFILER_ENTRIES),
      (request.interval || DEFAULT_PROFILER_INTERVAL),
      (request.features || DEFAULT_PROFILER_FEATURES),
      (request.features || DEFAULT_PROFILER_FEATURES).length,
      (request.threadFilters || DEFAULT_PROFILER_THREADFILTERS),
      (request.threadFilters || DEFAULT_PROFILER_THREADFILTERS).length);

    return { started: true };
  },

  


  onStopProfiler: function() {
    
    
    
    
    if (gProfilerConsumers == 1) {
      nsIProfilerModule.StopProfiler();
    }
    return { started: false };
  },

  



  onIsActive: function() {
    let isActive = nsIProfilerModule.IsActive();
    let elapsedTime = isActive ? nsIProfilerModule.getElapsedTime() : undefined;
    return { isActive: isActive, currentTime: elapsedTime };
  },

  




  onGetSharedLibraryInformation: function() {
    return { sharedLibraryInformation: nsIProfilerModule.getSharedLibraryInformation() };
  },

  






























  onGetProfile: function(request) {
    let startTime = request.startTime || 0;
    let profile = nsIProfilerModule.getProfileData(startTime);
    return { profile: profile, currentTime: nsIProfilerModule.getElapsedTime() };
  },

  






  onRegisterEventNotifications: function(request) {
    let response = [];
    for (let event of request.events) {
      if (this._observedEvents.has(event)) {
        continue;
      }
      Services.obs.addObserver(this, event, false);
      this._observedEvents.add(event);
      response.push(event);
    }
    return { registered: response };
  },

  






  onUnregisterEventNotifications: function(request) {
    let response = [];
    for (let event of request.events) {
      if (!this._observedEvents.has(event)) {
        continue;
      }
      Services.obs.removeObserver(this, event);
      this._observedEvents.delete(event);
      response.push(event);
    }
    return { unregistered: response };
  },

  





  observe: DevToolsUtils.makeInfallible(function(subject, topic, data) {
    
    
    
    
    
    subject = (subject && !Cu.isXrayWrapper(subject) && subject.wrappedJSObject) || subject;
    subject = JSON.parse(JSON.stringify(subject, cycleBreaker));
    data = (data && !Cu.isXrayWrapper(data) && data.wrappedJSObject) || data;
    data = JSON.parse(JSON.stringify(data, cycleBreaker));

    
    
    let reply = details => {
      this.conn.send({
        from: this.actorID,
        type: "eventNotification",
        subject: subject,
        topic: topic,
        data: data,
        details: details
      });
    };

    switch (topic) {
      case "console-api-profiler":
        return void reply(this._handleConsoleEvent(subject, data));
      case "profiler-started":
      case "profiler-stopped":
      default:
        return void reply();
    }
  }, "ProfilerActor.prototype.observe"),

  






  _handleConsoleEvent: function(subject, data) {
    
    
    let { action, arguments: args } = subject;
    let profileLabel = args.length > 0 ? args[0] + "" : undefined;

    
    
    

    if (action === "profile" || action === "profileEnd") {
      let { isActive, currentTime } = this.onIsActive();

      
      
      if (!isActive && action === "profile") {
        this.onStartProfiler();
        return {
          profileLabel: profileLabel,
          currentTime: 0
        };
      }
      
      
      else if (!isActive) {
        return {};
      }

      
      
      
      return {
        profileLabel: profileLabel,
        currentTime: currentTime
      };
    }
  }
};

exports.ProfilerActor = ProfilerActor;




function cycleBreaker(key, value) {
  if (key == "wrappedJSObject") {
    return undefined;
  }
  return value;
}




function checkProfilerConsumers() {
  if (gProfilerConsumers < 0) {
    let msg = "Somehow the number of started profilers is now negative.";
    DevToolsUtils.reportException("ProfilerActor", msg);
  }
}







ProfilerActor.prototype.requestTypes = {
  "getFeatures": ProfilerActor.prototype.onGetFeatures,
  "startProfiler": ProfilerActor.prototype.onStartProfiler,
  "stopProfiler": ProfilerActor.prototype.onStopProfiler,
  "isActive": ProfilerActor.prototype.onIsActive,
  "getSharedLibraryInformation": ProfilerActor.prototype.onGetSharedLibraryInformation,
  "getProfile": ProfilerActor.prototype.onGetProfile,
  "registerEventNotifications": ProfilerActor.prototype.onRegisterEventNotifications,
  "unregisterEventNotifications": ProfilerActor.prototype.onUnregisterEventNotifications
};
