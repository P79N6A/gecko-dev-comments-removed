



"use strict";



















const {Ci, Cu} = require("chrome");
const protocol = require("devtools/server/protocol");
const {method, Arg, RetVal} = protocol;
const events = require("sdk/event/core");
const {setTimeout, clearTimeout} = require("sdk/timers");

const TIMELINE_DATA_PULL_TIMEOUT = 300;

exports.register = function(handle) {
  handle.addGlobalActor(TimelineActor, "timelineActor");
  handle.addTabActor(TimelineActor, "timelineActor");
};

exports.unregister = function(handle) {
  handle.removeGlobalActor(TimelineActor);
  handle.removeTabActor(TimelineActor);
};





let TimelineActor = protocol.ActorClass({
  typeName: "timeline",

  events: {
    






    "markers" : {
      type: "markers",
      markers: Arg(0, "array:json")
    }
  },

  initialize: function(conn, tabActor) {
    protocol.Actor.prototype.initialize.call(this, conn);
    this.docshell = tabActor.docShell;
  },

  




  disconnect: function() {
    this.destroy();
  },

  destroy: function() {
    this.stop();
    this.docshell = null;
    protocol.Actor.prototype.destroy.call(this);
  },

  



  _pullTimelineData: function() {
    let markers = this.docshell.popProfileTimelineMarkers();
    if (markers.length > 0) {
      events.emit(this, "markers", markers);
    }
    this._dataPullTimeout = setTimeout(() => this._pullTimelineData(),
                                       TIMELINE_DATA_PULL_TIMEOUT);
  },

  


  isRecording: method(function() {
    return this.docshell.recordProfileTimelineMarkers;
  }, {
    request: {},
    response: {
      value: RetVal("boolean")
    }
  }),

  


  start: method(function() {
    if (!this.docshell.recordProfileTimelineMarkers) {
      this.docshell.recordProfileTimelineMarkers = true;
      this._pullTimelineData();
    }
  }, {oneway: true}),

  stop: method(function() {
    if (this.docshell.recordProfileTimelineMarkers) {
      this.docshell.recordProfileTimelineMarkers = false;
      clearTimeout(this._dataPullTimeout);
    }
  }, {oneway: true}),
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
