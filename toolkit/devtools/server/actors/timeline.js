



"use strict";


















const {Ci, Cu} = require("chrome");
const protocol = require("devtools/server/protocol");
const {method, Arg, RetVal, Option} = protocol;
const events = require("sdk/event/core");
const {setTimeout, clearTimeout} = require("sdk/timers");
const {MemoryActor} = require("devtools/server/actors/memory");
const {FramerateActor} = require("devtools/server/actors/framerate");




const DEFAULT_TIMELINE_DATA_PULL_TIMEOUT = 200; 




let TimelineActor = exports.TimelineActor = protocol.ActorClass({
  typeName: "timeline",

  events: {
    







    "markers" : {
      type: "markers",
      markers: Arg(0, "array:json"),
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
      timestamps: Arg(1, "array:number")
    }
  },

  initialize: function(conn, tabActor) {
    protocol.Actor.prototype.initialize.call(this, conn);
    this.tabActor = tabActor;

    this._isRecording = false;
    this._startTime = 0;

    
    this._onWindowReady = this._onWindowReady.bind(this);
    events.on(this.tabActor, "window-ready", this._onWindowReady);
  },

  




  disconnect: function() {
    this.destroy();
  },

  destroy: function() {
    this.stop();

    events.off(this.tabActor, "window-ready", this._onWindowReady);
    this.tabActor = null;

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
    if (!this._isRecording) {
      return;
    }
    if (!this.docShells.length) {
      return;
    }

    let endTime = this.docShells[0].now();
    let markers = [];

    for (let docShell of this.docShells) {
      markers = [...markers, ...docShell.popProfileTimelineMarkers()];
    }

    if (markers.length > 0) {
      events.emit(this, "markers", markers, endTime);
    }
    if (this._memoryActor) {
      events.emit(this, "memory", endTime, this._memoryActor.measure());
    }
    if (this._framerateActor) {
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

  


  start: method(function({ withMemory, withTicks }) {
    if (this._isRecording) {
      return;
    }
    this._isRecording = true;
    this._startTime = this.docShells[0].now();

    for (let docShell of this.docShells) {
      docShell.recordProfileTimelineMarkers = true;
    }

    if (withMemory) {
      this._memoryActor = new MemoryActor(this.conn, this.tabActor);
      events.emit(this, "memory", this._startTime, this._memoryActor.measure());
    }
    if (withTicks) {
      this._framerateActor = new FramerateActor(this.conn, this.tabActor);
      this._framerateActor.startRecording();
    }

    this._pullTimelineData();
    return this._startTime;
  }, {
    request: {
      withMemory: Option(0, "boolean"),
      withTicks: Option(0, "boolean")
    },
    response: {
      value: RetVal("number")
    }
  }),

  


  stop: method(function() {
    if (!this._isRecording) {
      return;
    }
    this._isRecording = false;

    if (this._memoryActor) {
      this._memoryActor = null;
    }
    if (this._framerateActor) {
      this._framerateActor.stopRecording();
      this._framerateActor = null;
    }

    for (let docShell of this.docShells) {
      docShell.recordProfileTimelineMarkers = false;
    }

    clearTimeout(this._dataPullTimeout);
  }, {}),

  



  _onWindowReady: function({window}) {
    if (this._isRecording) {
      let docShell = window.QueryInterface(Ci.nsIInterfaceRequestor)
                           .getInterface(Ci.nsIWebNavigation)
                           .QueryInterface(Ci.nsIDocShell);
      docShell.recordProfileTimelineMarkers = true;
    }
  }
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
