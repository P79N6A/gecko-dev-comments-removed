



"use strict";


















const {Ci, Cu} = require("chrome");
const protocol = require("devtools/server/protocol");
const {method, Arg, RetVal} = protocol;
const events = require("sdk/event/core");
const {setTimeout, clearTimeout} = require("sdk/timers");




const DEFAULT_TIMELINE_DATA_PULL_TIMEOUT = 200; 




let TimelineActor = exports.TimelineActor = protocol.ActorClass({
  typeName: "timeline",

  events: {
    







    "markers" : {
      type: "markers",
      markers: Arg(0, "array:json")
    }
  },

  initialize: function(conn, tabActor) {
    protocol.Actor.prototype.initialize.call(this, conn);
    this.tabActor = tabActor;

    this._isRecording = false;

    
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

  




  toDocShell: win => win.QueryInterface(Ci.nsIInterfaceRequestor)
                        .getInterface(Ci.nsIWebNavigation)
                        .QueryInterface(Ci.nsIDocShell),

  



  get docShells() {
    return this.tabActor.windows.map(this.toDocShell);
  },

  



  _pullTimelineData: function() {
    if (!this._isRecording) {
      return;
    }

    let markers = [];
    for (let docShell of this.docShells) {
      markers = [...markers, ...docShell.popProfileTimelineMarkers()];
    }
    if (markers.length > 0) {
      events.emit(this, "markers", markers);
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

  


  start: method(function() {
    if (this._isRecording) {
      return;
    }
    this._isRecording = true;

    for (let docShell of this.docShells) {
      docShell.recordProfileTimelineMarkers = true;
    }

    this._pullTimelineData();
  }, {}),

  


  stop: method(function() {
    if (!this._isRecording) {
      return;
    }
    this._isRecording = false;

    for (let docShell of this.docShells) {
      docShell.recordProfileTimelineMarkers = false;
    }

    clearTimeout(this._dataPullTimeout);
  }, {}),

  



  _onWindowReady: function({window}) {
    if (this._isRecording) {
      this.toDocShell(window).recordProfileTimelineMarkers = true;
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
