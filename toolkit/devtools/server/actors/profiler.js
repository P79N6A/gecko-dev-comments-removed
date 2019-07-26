



"use strict";

var startedProfilers = 0;
var startTime = 0;

function getCurrentTime() {
  return (new Date()).getTime() - startTime;
}




































function ProfilerActor(aConnection)
{
  this._profiler = Cc["@mozilla.org/tools/profiler;1"].getService(Ci.nsIProfiler);
  this._started = false;
  this._observedEvents = [];
}

ProfilerActor.prototype = {
  actorPrefix: "profiler",

  disconnect: function() {
    for (var event of this._observedEvents) {
      Services.obs.removeObserver(this, event);
    }

    this.stopProfiler();
    this._profiler = null;
  },

  stopProfiler: function() {
    
    
    
    
    
    if (!this._started) {
      return;
    }
    this._started = false;
    startedProfilers -= 1;
    if (startedProfilers <= 0) {
      this._profiler.StopProfiler();
    }
  },

  onStartProfiler: function(aRequest) {
    this._profiler.StartProfiler(aRequest.entries, aRequest.interval,
                           aRequest.features, aRequest.features.length);
    this._started = true;
    startedProfilers += 1;
    startTime = (new Date()).getTime();
    return { "msg": "profiler started" }
  },
  onStopProfiler: function(aRequest) {
    this.stopProfiler();
    return { "msg": "profiler stopped" }
  },
  onGetProfileStr: function(aRequest) {
    var profileStr = this._profiler.GetProfile();
    return { "profileStr": profileStr }
  },
  onGetProfile: function(aRequest) {
    var profile = this._profiler.getProfileData();
    return { "profile": profile, "currentTime": getCurrentTime() }
  },
  onIsActive: function(aRequest) {
    var isActive = this._profiler.IsActive();
    var currentTime = isActive ? getCurrentTime() : null;
    return { "isActive": isActive, "currentTime": currentTime }
  },
  onGetFeatures: function(aRequest) {
    var features = this._profiler.GetFeatures([]);
    return { "features": features }
  },
  onGetSharedLibraryInformation: function(aRequest) {
    var sharedLibraries = this._profiler.getSharedLibraryInformation();
    return { "sharedLibraryInformation": sharedLibraries }
  },
  onRegisterEventNotifications: function(aRequest) {
    let registered = [];
    for (var event of aRequest.events) {
      if (this._observedEvents.indexOf(event) != -1)
        continue;
      Services.obs.addObserver(this, event, false);
      this._observedEvents.push(event);
      registered.push(event);
    }
    return { registered: registered }
  },
  onUnregisterEventNotifications: function(aRequest) {
    let unregistered = [];
    for (var event of aRequest.events) {
      let idx = this._observedEvents.indexOf(event);
      if (idx == -1)
        continue;
      Services.obs.removeObserver(this, event);
      this._observedEvents.splice(idx, 1);
      unregistered.push(event);
    }
    return { unregistered: unregistered }
  },
  observe: DevToolsUtils.makeInfallible(function(aSubject, aTopic, aData) {
    














    function cycleBreaker(key, value) {
      if (key === 'wrappedJSObject') {
        return undefined;
      }
      return value;
    }

    




    aSubject = (aSubject && !Cu.isXrayWrapper(aSubject) && aSubject.wrappedJSObject) || aSubject;
    aData    = (aData && !Cu.isXrayWrapper(aData) && aData.wrappedJSObject) || aData;

    let subj = JSON.parse(JSON.stringify(aSubject, cycleBreaker));
    let data = JSON.parse(JSON.stringify(aData,    cycleBreaker));

    let send = (extra) => {
      data = data || {};

      if (extra)
        data.extra = extra;

      this.conn.send({
        from:    this.actorID,
        type:    "eventNotification",
        event:   aTopic,
        subject: subj,
        data:    data
      });
    }

    if (aTopic !== "console-api-profiler")
      return void send();

    
    
    
    

    let name = subj.arguments[0];

    if (subj.action === "profile") {
      let resp = this.onIsActive();

      if (resp.isActive) {
        return void send({
          name: name,
          currentTime: resp.currentTime,
          action: "profile"
        });
      }

      this.onStartProfiler({
        entries: 1000000,
        interval: 1,
        features: ["js"]
      });

      return void send({ currentTime: 0, action: "profile", name: name });
    }

    if (subj.action === "profileEnd") {
      let resp = this.onGetProfile();
      resp.action = "profileEnd";
      resp.name = name;
      send(resp);
    }

    return undefined; 
  }, "ProfilerActor.prototype.observe"),
};




ProfilerActor.prototype.requestTypes = {
  "startProfiler": ProfilerActor.prototype.onStartProfiler,
  "stopProfiler": ProfilerActor.prototype.onStopProfiler,
  "getProfileStr": ProfilerActor.prototype.onGetProfileStr,
  "getProfile": ProfilerActor.prototype.onGetProfile,
  "isActive": ProfilerActor.prototype.onIsActive,
  "getFeatures": ProfilerActor.prototype.onGetFeatures,
  "getSharedLibraryInformation": ProfilerActor.prototype.onGetSharedLibraryInformation,
  "registerEventNotifications": ProfilerActor.prototype.onRegisterEventNotifications,
  "unregisterEventNotifications": ProfilerActor.prototype.onUnregisterEventNotifications
};

DebuggerServer.addGlobalActor(ProfilerActor, "profilerActor");
