


"use strict";

















const {Ci, Cu} = require("chrome");
const protocol = require("devtools/server/protocol");
const {method, Arg, RetVal, Option} = protocol;
const events = require("sdk/event/core");
const {setTimeout, clearTimeout} = require("sdk/timers");
const {Task} = Cu.import("resource://gre/modules/Task.jsm", {});

const {MemoryBridge} = require("devtools/server/actors/utils/memory-bridge");
const {FramerateActor} = require("devtools/server/actors/framerate");
const {StackFrameCache} = require("devtools/server/actors/utils/stack");




const DEFAULT_TIMELINE_DATA_PULL_TIMEOUT = 200; 









protocol.types.addType("array-of-numbers-as-strings", {
  write: (v) => v.join(","),
  
  read: (v) => typeof v === "string" ? v.split(",") : v
});




let TimelineActor = exports.TimelineActor = protocol.ActorClass({
  typeName: "timeline",

  events: {
    




    "markers" : {
      type: "markers",
      markers: Arg(0, "json"),
      endTime: Arg(1, "number")
    },

    




    "memory" : {
      type: "memory",
      delta: Arg(0, "number"),
      measurement: Arg(1, "json")
    },

    




    "ticks" : {
      type: "ticks",
      delta: Arg(0, "number"),
      timestamps: Arg(1, "array-of-numbers-as-strings")
    },

    




    "frames" : {
      type: "frames",
      delta: Arg(0, "number"),
      frames: Arg(1, "json")
    }
  },

  


  initialize: function(conn, tabActor) {
    protocol.Actor.prototype.initialize.call(this, conn);
    this.tabActor = tabActor;

    this._isRecording = false;
    this._stackFrames = null;
    this._memoryBridge = null;

    
    this._onWindowReady = this._onWindowReady.bind(this);
    this._onGarbageCollection = this._onGarbageCollection.bind(this);
    events.on(this.tabActor, "window-ready", this._onWindowReady);
  },

  




  disconnect: function() {
    this.destroy();
  },

  


  destroy: function() {
    this.stop();

    events.off(this.tabActor, "window-ready", this._onWindowReady);
    this.tabActor = null;
    this._memoryBridge = null;

    protocol.Actor.prototype.destroy.call(this);
  },

  








  get docShells() {
    let originalDocShell;

    if (this.tabActor.isRootActor) {
      originalDocShell = this.tabActor.docShell;
    } else {
      originalDocShell = this.tabActor.originalDocShell;
    }

    let docShellsEnum = originalDocShell.getDocShellEnumerator(
      Ci.nsIDocShellTreeItem.typeAll,
      Ci.nsIDocShell.ENUMERATE_FORWARDS
    );

    let docShells = [];
    while (docShellsEnum.hasMoreElements()) {
      let docShell = docShellsEnum.getNext();
      docShells.push(docShell.QueryInterface(Ci.nsIDocShell));
    }

    return docShells;
  },

  



  _pullTimelineData: function() {
    if (!this._isRecording || !this.docShells.length) {
      return;
    }

    let endTime = this.docShells[0].now();
    let markers = [];

    for (let docShell of this.docShells) {
      markers.push(...docShell.popProfileTimelineMarkers());
    }

    
    
    
    
    
    
    for (let marker of markers) {
      if (marker.stack) {
        marker.stack = this._stackFrames.addFrame(Cu.waiveXrays(marker.stack));
      }
      if (marker.endStack) {
        marker.endStack = this._stackFrames.addFrame(Cu.waiveXrays(marker.endStack));
      }
    }

    let frames = this._stackFrames.makeEvent();
    if (frames) {
      events.emit(this, "frames", endTime, frames);
    }
    if (markers.length > 0) {
      events.emit(this, "markers", markers, endTime);
    }
    if (this._withMemory) {
      events.emit(this, "memory", endTime, this._memoryBridge.measure());
    }
    if (this._withTicks) {
      events.emit(this, "ticks", endTime, this._framerateActor.getPendingTicks());
    }

    this._dataPullTimeout = setTimeout(() => {
      this._pullTimelineData();
    }, DEFAULT_TIMELINE_DATA_PULL_TIMEOUT);
  },

  


  isRecording: method(function() {
    return this._isRecording;
  }, {
    request: {},
    response: {
      value: RetVal("boolean")
    }
  }),

  










  start: method(Task.async(function *({ withMemory, withTicks }) {
    var startTime = this._startTime = this.docShells[0].now();
    
    
    this._unixStartTime = Date.now();

    if (this._isRecording) {
      return startTime;
    }

    this._isRecording = true;
    this._stackFrames = new StackFrameCache();
    this._stackFrames.initFrames();
    this._withMemory = withMemory;
    this._withTicks = withTicks;

    for (let docShell of this.docShells) {
      docShell.recordProfileTimelineMarkers = true;
    }

    this._memoryBridge = new MemoryBridge(this.tabActor, this._stackFrames);
    this._memoryBridge.attach();
    events.on(this._memoryBridge, "garbage-collection", this._onGarbageCollection);

    if (withTicks) {
      this._framerateActor = new FramerateActor(this.conn, this.tabActor);
      this._framerateActor.startRecording();
    }

    this._pullTimelineData();
    return startTime;
  }), {
    request: {
      withMemory: Option(0, "boolean"),
      withTicks: Option(0, "boolean")
    },
    response: {
      value: RetVal("number")
    }
  }),

  


  stop: method(Task.async(function *() {
    if (!this._isRecording) {
      return;
    }
    this._isRecording = false;
    this._stackFrames = null;

    events.off(this._memoryBridge, "garbage-collection", this._onGarbageCollection);
    this._memoryBridge.detach();

    if (this._framerateActor) {
      this._framerateActor.stopRecording();
      this._framerateActor = null;
    }

    for (let docShell of this.docShells) {
      docShell.recordProfileTimelineMarkers = false;
    }

    clearTimeout(this._dataPullTimeout);
    return this.docShells[0].now();
  }), {
    response: {
      
      
      value: RetVal("nullable:number")
    }
  }),

  



  _onWindowReady: function({window}) {
    if (this._isRecording) {
      let docShell = window.QueryInterface(Ci.nsIInterfaceRequestor)
                           .getInterface(Ci.nsIWebNavigation)
                           .QueryInterface(Ci.nsIDocShell);
      docShell.recordProfileTimelineMarkers = true;
    }
  },

  








  _onGarbageCollection: function ({ collections, reason, nonincrementalReason }) {
    if (!this._isRecording || !this.docShells.length) {
      return;
    }

    
    
    let startTime = (this._unixStartTime - this._startTime) * 1000;
    let endTime = this.docShells[0].now();

    events.emit(this, "markers", collections.map(({ startTimestamp: start, endTimestamp: end }) => {
      return {
        name: "GarbageCollection",
        causeName: reason,
        nonincrementalReason: nonincrementalReason,
        
        start: (start - startTime) / 1000,
        end: (end - startTime) / 1000
      };
    }), endTime);
  },
});

exports.TimelineFront = protocol.FrontClass(TimelineActor, {
  initialize: function(client, {timelineActor}) {
    protocol.Front.prototype.initialize.call(this, client, {actor: timelineActor});
    this.manage(this);
  },
  destroy: function() {
    protocol.Front.prototype.destroy.call(this);
  },
});
