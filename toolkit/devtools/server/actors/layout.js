



"use strict";





















const {Ci, Cu} = require("chrome");
Cu.import("resource://gre/modules/XPCOMUtils.jsm");
const protocol = require("devtools/server/protocol");
const {method, Arg, RetVal, types} = protocol;
const events = require("sdk/event/core");
const Heritage = require("sdk/core/heritage");
const EventEmitter = require("devtools/toolkit/event-emitter");

exports.register = function(handle) {
  handle.addGlobalActor(ReflowActor, "reflowActor");
  handle.addTabActor(ReflowActor, "reflowActor");
};

exports.unregister = function(handle) {
  handle.removeGlobalActor(ReflowActor);
  handle.removeTabActor(ReflowActor);
};




let ReflowActor = protocol.ActorClass({
  typeName: "reflow",

  events: {
    







    "reflows" : {
      type: "reflows",
      reflows: Arg(0, "array:json")
    }
  },

  initialize: function(conn, tabActor) {
    protocol.Actor.prototype.initialize.call(this, conn);

    this.tabActor = tabActor;
    this._onReflow = this._onReflow.bind(this);
    this.observer = getLayoutChangesObserver(tabActor);
    this._isStarted = false;
  },

  




  disconnect: function() {
    this.destroy();
  },

  destroy: function() {
    this.stop();
    releaseLayoutChangesObserver(this.tabActor);
    this.observer = null;
    this.tabActor = null;

    protocol.Actor.prototype.destroy.call(this);
  },

  




  start: method(function() {
    if (!this._isStarted) {
      this.observer.on("reflows", this._onReflow);
      this._isStarted = true;
    }
  }, {oneway: true}),

  




  stop: method(function() {
    if (this._isStarted) {
      this.observer.off("reflows", this._onReflow);
      this._isStarted = false;
    }
  }, {oneway: true}),

  _onReflow: function(event, reflows) {
    if (this._isStarted) {
      events.emit(this, "reflows", reflows);
    }
  }
});









exports.ReflowFront = protocol.FrontClass(ReflowActor, {
  initialize: function(client, {reflowActor}) {
    protocol.Front.prototype.initialize.call(this, client, {actor: reflowActor});
    client.addActorPool(this);
    this.manage(this);
  },

  destroy: function() {
    protocol.Front.prototype.destroy.call(this);
  },
});






function Observable(tabActor, callback) {
  this.tabActor = tabActor;
  this.win = tabActor.window;
  this.callback = callback;
}

Observable.prototype = {
  


  observing: false,

  


  start: function() {
    if (!this.observing) {
      this._start();
      this.observing = true;
    }
  },

  _start: function() {
    
  },

  


  stop: function() {
    if (this.observing) {
      this._stop();
      this.observing = false;
    }
  },

  _stop: function() {
    
  },

  


  notifyCallback: function(...args) {
    this.observing && this.callback && this.callback.apply(null, args);
  },

  


  destroy: function() {
    this.stop();
    this.callback = null;
    this.win = null;
    this.tabActor = null;
  }
};




















function LayoutChangesObserver(tabActor) {
  Observable.call(this, tabActor);

  this._startEventLoop = this._startEventLoop.bind(this);

  
  
  
  this._onReflow = this._onReflow.bind(this);
  this.reflowObserver = new ReflowObserver(this.tabActor, this._onReflow);

  EventEmitter.decorate(this);
}

LayoutChangesObserver.prototype = Heritage.extend(Observable.prototype, {
  






  EVENT_BATCHING_DELAY: 300,

  



  destroy: function() {
    this.reflowObserver.destroy();
    this.reflows = null;

    Observable.prototype.destroy.call(this);
  },

  _start: function() {
    this.reflows = [];
    this._startEventLoop();
    this.reflowObserver.start();
  },

  _stop: function() {
    this._stopEventLoop();
    this.reflows = [];
    this.reflowObserver.stop();
  },

  




  _startEventLoop: function() {
    
    if (this.reflows && this.reflows.length) {
      this.emit("reflows", this.reflows);
      this.reflows = [];
    }
    this.eventLoopTimer = this.win.setTimeout(this._startEventLoop,
      this.EVENT_BATCHING_DELAY);
  },

  _stopEventLoop: function() {
    this.win.clearTimeout(this.eventLoopTimer);
  },

  







  _onReflow: function(start, end, isInterruptible) {
    
    
    this.reflows.push({
      start: start,
      end: end,
      isInterruptible: isInterruptible
    });
  }
});







let observedWindows = new Map();
function getLayoutChangesObserver(tabActor) {
  let observerData = observedWindows.get(tabActor);
  if (observerData) {
    observerData.refCounting ++;
    return observerData.observer;
  }

  let obs = new LayoutChangesObserver(tabActor);
  observedWindows.set(tabActor, {
    observer: obs,
    refCounting: 1 
                   
  });
  obs.start();
  return obs;
};
exports.getLayoutChangesObserver = getLayoutChangesObserver;







function releaseLayoutChangesObserver(tabActor) {
  let observerData = observedWindows.get(tabActor);
  if (!observerData) {
    return;
  }

  observerData.refCounting --;
  if (!observerData.refCounting) {
    observerData.observer.destroy();
    observedWindows.delete(tabActor);
  }
};
exports.releaseLayoutChangesObserver = releaseLayoutChangesObserver;








function ReflowObserver(tabActor, callback) {
  Observable.call(this, tabActor, callback);
  this.docshell = this.win.QueryInterface(Ci.nsIInterfaceRequestor)
                     .getInterface(Ci.nsIWebNavigation)
                     .QueryInterface(Ci.nsIDocShell);
}

ReflowObserver.prototype = Heritage.extend(Observable.prototype, {
  QueryInterface: XPCOMUtils.generateQI([Ci.nsIReflowObserver,
    Ci.nsISupportsWeakReference]),

  _start: function() {
    this.docshell.addWeakReflowObserver(this);
  },

  _stop: function() {
    this.docshell.removeWeakReflowObserver(this);
  },

  reflow: function(start, end) {
    this.notifyCallback(start, end, false);
  },

  reflowInterruptible: function(start, end) {
    this.notifyCallback(start, end, true);
  },

  destroy: function() {
    Observable.prototype.destroy.call(this);
    this.docshell = null;
  }
});
