


"use strict";

const {Cc, Ci, Cu, Cr} = require("chrome");

Cu.import("resource://gre/modules/Task.jsm");

loader.lazyRequireGetter(this, "Services");
loader.lazyRequireGetter(this, "promise");
loader.lazyRequireGetter(this, "EventEmitter",
  "devtools/toolkit/event-emitter");
loader.lazyRequireGetter(this, "FramerateFront",
  "devtools/server/actors/framerate", true);
loader.lazyRequireGetter(this, "DevToolsUtils",
  "devtools/toolkit/DevToolsUtils");

loader.lazyImporter(this, "gDevTools",
  "resource:///modules/devtools/gDevTools.jsm");




let SharedProfilerConnection = new WeakMap();








SharedProfilerConnection.forToolbox = function(toolbox) {
  if (this.has(toolbox)) {
    return this.get(toolbox);
  }

  let instance = new ProfilerConnection(toolbox);
  this.set(toolbox, instance);
  return instance;
};











function ProfilerConnection(toolbox) {
  EventEmitter.decorate(this);

  this._toolbox = toolbox;
  this._target = this._toolbox.target;
  this._client = this._target.client;
  this._request = this._request.bind(this);

  this._pendingFramerateConsumers = 0;
  this._pendingConsoleRecordings = [];
  this._finishedConsoleRecordings = [];
  this._onEventNotification = this._onEventNotification.bind(this);

  Services.obs.notifyObservers(null, "profiler-connection-created", null);
}

ProfilerConnection.prototype = {
  






  open: Task.async(function*() {
    if (this._connected) {
      return;
    }

    
    yield this._target.makeRemote();

    
    
    if (this._target.chrome) {
      this._profiler = this._target.form.profilerActor;
    }
    
    
    else if (this._target.form && this._target.form.profilerActor) {
      this._profiler = this._target.form.profilerActor;
      yield this._registerEventNotifications();
    }
    
    
    else if (this._target.root && this._target.root.profilerActor) {
      this._profiler = this._target.root.profilerActor;
      yield this._registerEventNotifications();
    }
    
    else {
      this._profiler = (yield listTabs(this._client)).profilerActor;
      yield this._registerEventNotifications();
    }

    this._connectMiscActors();
    this._connected = true;

    Services.obs.notifyObservers(null, "profiler-connection-opened", null);
  }),

  



  _connectMiscActors: function() {
    
    
    
    if (this._target.form && this._target.form.framerateActor) {
    this._framerate = new FramerateFront(this._target.client, this._target.form);
    } else {
      this._framerate = {
        startRecording: () => {},
        stopRecording: () => {},
        cancelRecording: () => {},
        isRecording: () => false,
        getPendingTicks: () => null
      };
    }
  },

  












  _request: function(actor, method, ...args) {
    
    if (actor == "profiler") {
      let deferred = promise.defer();
      let data = args[0] || {};
      data.to = this._profiler;
      data.type = method;
      this._client.request(data, deferred.resolve);
      return deferred.promise;
    }

    
    if (actor == "framerate") {
      switch (method) {
      
      
      
      
        case "startRecording":
          this._pendingFramerateConsumers++;
          break;
        case "stopRecording":
        case "cancelRecording":
          if (--this._pendingFramerateConsumers > 0) return;
          break;
        
        
        
        case "getPendingTicks":
          if (method in this._framerate) break;
          return null;
      }
      checkPendingFramerateConsumers(this);
      return this._framerate[method].apply(this._framerate, args);
    }
  },

  





  _registerEventNotifications: Task.async(function*() {
    let events = ["console-api-profiler", "profiler-stopped"];
    yield this._request("profiler", "registerEventNotifications", { events });
    this._client.addListener("eventNotification", this._onEventNotification);
  }),

  





  _onEventNotification: function(event, response) {
    let toolbox = gDevTools.getToolbox(this._target);
    if (toolbox == null) {
      return;
    }
    if (response.topic == "console-api-profiler") {
      let action = response.subject.action;
      let details = response.details;
      if (action == "profile") {
        this.emit("invoked-console-profile", details.profileLabel); 
        this._onConsoleProfileStart(details);
      } else if (action == "profileEnd") {
        this.emit("invoked-console-profileEnd", details.profileLabel); 
        this._onConsoleProfileEnd(details);
      }
    } else if (response.topic == "profiler-stopped") {
      this._onProfilerUnexpectedlyStopped();
    }
  },

  








  _onConsoleProfileStart: Task.async(function*({ profileLabel, currentTime }) {
    let pending = this._pendingConsoleRecordings;
    if (pending.find(e => e.profileLabel == profileLabel)) {
      return;
    }
    
    pending.push({
      profileLabel: profileLabel,
      profilingStartTime: currentTime
    });

    
    
    yield this._request("framerate", "startRecording");

    
    this.emit("profile", profileLabel);
  }),

  





  _onConsoleProfileEnd: Task.async(function*(profilerData) {
    let pending = this._pendingConsoleRecordings;
    if (pending.length == 0) {
      return;
    }
    
    
    let info = pending.find(e => e.profileLabel == profilerData.profileLabel);
    if (info) {
      pending.splice(pending.indexOf(info), 1);
    }
    
    
    else if (!profilerData.profileLabel) {
      info = pending.pop();
      profilerData.profileLabel = info.profileLabel;
    }
    
    
    else {
      return;
    }

    
    
    
    filterSamples(profilerData, info.profilingStartTime);
    offsetSampleTimes(profilerData, info.profilingStartTime);

    
    
    let beginAt = findEarliestSampleTime(profilerData);
    let endAt = findOldestSampleTime(profilerData);
    let ticksData = yield this._request("framerate", "getPendingTicks", beginAt, endAt);
    yield this._request("framerate", "cancelRecording");

    
    let recordingData = {
      recordingDuration: profilerData.currentTime - info.profilingStartTime,
      profilerData: profilerData,
      ticksData: ticksData
    };
    this._finishedConsoleRecordings.push(recordingData);

    
    this.emit("profileEnd", recordingData);
  }),

  








  _onProfilerUnexpectedlyStopped: function() {
    
    this._pendingConsoleRecordings.length = 0;
    this.emit("profiler-unexpectedly-stopped");
  }
};








function ProfilerFront(connection) {
  EventEmitter.decorate(this);

  this._request = connection._request;
  this.pendingConsoleRecordings = connection._pendingConsoleRecordings;
  this.finishedConsoleRecordings = connection._finishedConsoleRecordings;

  connection.on("profile", (e, args) => this.emit(e, args));
  connection.on("profileEnd", (e, args) => this.emit(e, args));
  connection.on("profiler-unexpectedly-stopped", (e, args) => this.emit(e, args));
}

ProfilerFront.prototype = {
  





  startRecording: Task.async(function*() {
    let { isActive, currentTime } = yield this._request("profiler", "isActive");

    
    
    
    
    if (!isActive) {
      yield this._request("profiler", "startProfiler", this._customProfilerOptions);
      this._profilingStartTime = 0;
      this.emit("profiler-activated");
    } else {
      this._profilingStartTime = currentTime;
      this.emit("profiler-already-active");
    }

    
    
    yield this._request("framerate", "startRecording");
  }),

  






  stopRecording: Task.async(function*() {
    
    
    let profilerData = yield this._request("profiler", "getProfile");
    filterSamples(profilerData, this._profilingStartTime);
    offsetSampleTimes(profilerData, this._profilingStartTime);

    
    
    let beginAt = findEarliestSampleTime(profilerData);
    let endAt = findOldestSampleTime(profilerData);
    let ticksData = yield this._request("framerate", "getPendingTicks", beginAt, endAt);
    yield this._request("framerate", "cancelRecording");

    
    return {
      recordingDuration: profilerData.currentTime - this._profilingStartTime,
      profilerData: profilerData,
      ticksData: ticksData
    };
  }),

  


  cancelRecording: Task.async(function*() {
    yield this._request("framerate", "cancelRecording");
  }),

  





  _customProfilerOptions: {
    entries: 1000000,
    interval: 1,
    features: ["js"]
  }
};










function filterSamples(profilerData, profilingStartTime) {
  let firstThread = profilerData.profile.threads[0];

  firstThread.samples = firstThread.samples.filter(e => {
    return e.time >= profilingStartTime;
  });
}









function offsetSampleTimes(profilerData, timeOffset) {
  let firstThreadSamples = profilerData.profile.threads[0].samples;

  for (let sample of firstThreadSamples) {
    sample.time -= timeOffset;
  }
}









function findEarliestSampleTime(profilerData) {
  let firstThreadSamples = profilerData.profile.threads[0].samples;

  for (let sample of firstThreadSamples) {
    if ("time" in sample) {
      return sample.time;
    }
  }
}









function findOldestSampleTime(profilerData) {
  let firstThreadSamples = profilerData.profile.threads[0].samples;

  for (let i = firstThreadSamples.length - 1; i >= 0; i--) {
    if ("time" in firstThreadSamples[i]) {
      return firstThreadSamples[i].time;
    }
  }
}




function checkPendingFramerateConsumers(connection) {
  if (connection._pendingFramerateConsumers < 0) {
    let msg = "Somehow the number of framerate consumers is now negative.";
    DevToolsUtils.reportException("ProfilerConnection", msg);
  }
}




function listTabs(client) {
  let deferred = promise.defer();
  client.listTabs(deferred.resolve);
  return deferred.promise;
}

exports.getProfilerConnection = toolbox => SharedProfilerConnection.forToolbox(toolbox);
exports.ProfilerFront = ProfilerFront;
