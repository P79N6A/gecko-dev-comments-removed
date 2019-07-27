



"use strict";

const { Cu } = require("chrome");
const protocol = require("devtools/server/protocol");
const { custom, method, RetVal, Arg, Option, types } = protocol;
const { Profiler } = require("devtools/toolkit/shared/profiler");
const { actorBridge } = require("devtools/server/actors/common");

loader.lazyRequireGetter(this, "events", "sdk/event/core");
loader.lazyRequireGetter(this, "extend", "sdk/util/object", true);

types.addType("profiler-data", {
  
  
  
  read: (v) => {
    if (typeof v.profile === "string") {
      
      let newValue = Object.create(null);
      newValue.profile = JSON.parse(v.profile);
      newValue.currentTime = v.currentTime;
      return newValue;
    }
    return v;
  }
});







let ProfilerActor = exports.ProfilerActor = protocol.ActorClass({
  typeName: "profiler",

  


  events: {
    "console-api-profiler": {
      data: Arg(0, "json"),
    },
    "profiler-started": {
      data: Arg(0, "json"),
    },
    "profiler-stopped": {
      data: Arg(0, "json"),
    },
    "profiler-status": {
      data: Arg(0, "json"),
    },

    
    
    
    "eventNotification": {
      subject: Option(0, "json"),
      topic: Option(0, "string"),
      details: Option(0, "json")
    }
  },

  initialize: function (conn) {
    protocol.Actor.prototype.initialize.call(this, conn);
    this._onProfilerEvent = this._onProfilerEvent.bind(this);

    this.bridge = new Profiler();
    events.on(this.bridge, "*", this._onProfilerEvent);
  },

  



  disconnect: function() {
    this.destroy();
  },

  destroy: function() {
    events.off(this.bridge, "*", this._onProfilerEvent);
    this.bridge.destroy();
    protocol.Actor.prototype.destroy.call(this);
  },

  startProfiler: actorBridge("start", {
    
    
    
    request: {
      entries: Option(0, "nullable:number"),
      interval: Option(0, "nullable:number"),
      features: Option(0, "nullable:array:string"),
      threadFilters: Option(0, "nullable:array:string"),
    },
    response: RetVal("json"),
  }),

  stopProfiler: actorBridge("stop", {
    response: RetVal("json"),
  }),

  getProfile: actorBridge("getProfile", {
    request: {
      startTime: Option(0, "nullable:number"),
      stringify: Option(0, "nullable:boolean")
    },
    response: RetVal("profiler-data")
  }),

  getFeatures: actorBridge("getFeatures", {
    response: RetVal("json")
  }),

  getBufferInfo: actorBridge("getBufferInfo", {
    response: RetVal("json")
  }),

  getStartOptions: actorBridge("getStartOptions", {
    response: RetVal("json")
  }),

  isActive: actorBridge("isActive", {
    response: RetVal("json")
  }),

  getSharedLibraryInformation: actorBridge("getSharedLibraryInformation", {
    response: RetVal("json")
  }),

  registerEventNotifications: actorBridge("registerEventNotifications", {
    
    
    request: {
      events: Option(0, "nullable:array:string"),
    },
    response: RetVal("json")
  }),

  unregisterEventNotifications: actorBridge("unregisterEventNotifications", {
    
    
    request: {
      events: Option(0, "nullable:array:string"),
    },
    response: RetVal("json")
  }),

  setProfilerStatusInterval: actorBridge("setProfilerStatusInterval", {
    request: { interval: Arg(0, "number") },
    oneway: true
  }),

  


  _onProfilerEvent: function (eventName, ...data) {
    events.emit(this, eventName, ...data);
  },
});





exports.ProfilerFront = protocol.FrontClass(ProfilerActor, {
  initialize: function(client, form) {
    protocol.Front.prototype.initialize.call(this, client, form);
    this.actorID = form.profilerActor;
    this.manage(this);

    this._onProfilerEvent = this._onProfilerEvent.bind(this);
    events.on(this, "*", this._onProfilerEvent);
  },

  destroy: function () {
    events.off(this, "*", this._onProfilerEvent);
    protocol.Front.prototype.destroy.call(this);
  },

  





  getProfile: custom(function (options) {
    return this._getProfile(extend({ stringify: true }, options));
  }, {
    impl: "_getProfile"
  }),

  


  _onProfilerEvent: function (eventName, data) {
    
    if (data.relayed) {
      return;
    }
    data.relayed = true;

    
    
    
    if (eventName === "eventNotification") {
      events.emit(this, data.topic, data);
    }
    
    
    
    else {
      this.conn.emit("eventNotification", {
        subject: data.subject,
        topic: data.topic,
        data: data.data,
        details: data.details
      });
      if (this.conn._getListeners("eventNotification").length) {
        Cu.reportError(`
          ProfilerActor's "eventNotification" on the DebuggerClient has been deprecated.
          Use the ProfilerFront found in "devtools/server/actors/profiler".`);
      }
    }
  },
});
