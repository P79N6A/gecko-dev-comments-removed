









const {Ci, Cu} = require("chrome");
const Services = require("Services");
Cu.import("resource://gre/modules/XPCOMUtils.jsm");
const protocol = require("devtools/server/protocol");
const {method, Arg, RetVal} = protocol;
const events = require("sdk/event/core");

exports.register = function(handle) {
  handle.addGlobalActor(EventLoopLagActor, "eventLoopLagActor");
  handle.addTabActor(EventLoopLagActor, "eventLoopLagActor");
};

exports.unregister = function(handle) {
  handle.removeGlobalActor(EventLoopLagActor);
  handle.removeTabActor(EventLoopLagActor);
};

let EventLoopLagActor = protocol.ActorClass({

  typeName: "eventLoopLag",

  _observerAdded: false,

  events: {
    "event-loop-lag" : {
      type: "event-loop-lag",
      time: Arg(0, "number") 
    }
  },

  


  start: method(function() {
    if (!this._observerAdded) {
      Services.obs.addObserver(this, 'event-loop-lag', false);
      this._observerAdded = true;
    }
    return Services.appShell.startEventLoopLagTracking();
  }, {
    request: {},
    response: {success: RetVal("number")}
  }),

  


  stop: method(function() {
    if (this._observerAdded) {
      Services.obs.removeObserver(this, 'event-loop-lag');
      this._observerAdded = false;
    }
    Services.appShell.stopEventLoopLagTracking();
  }, {request: {},response: {}}),

  destroy: function() {
    this.stop();
    protocol.Actor.prototype.destroy.call(this);
  },

  

  observe: function (subject, topic, data) {
    if (topic == "event-loop-lag") {
      
      events.emit(this, "event-loop-lag", data);
    }
  },

  QueryInterface: XPCOMUtils.generateQI([Ci.nsIObserver]),
});

exports.EventLoopLagFront = protocol.FrontClass(EventLoopLagActor, {
  initialize: function(client, form) {
    protocol.Front.prototype.initialize.call(this, client);
    this.actorID = form.eventLoopLagActor;
    this.manage(this);
  },
});
