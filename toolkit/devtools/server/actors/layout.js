



"use strict";





















const {Ci, Cu} = require("chrome");
Cu.import("resource://gre/modules/XPCOMUtils.jsm");
const protocol = require("devtools/server/protocol");
const {method, Arg, RetVal, types} = protocol;
const events = require("sdk/event/core");
const Heritage = require("sdk/core/heritage");
const {setTimeout, clearTimeout} = require("sdk/timers");
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

exports.LayoutChangesObserver = LayoutChangesObserver;

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
    this.eventLoopTimer = this._setTimeout(this._startEventLoop,
      this.EVENT_BATCHING_DELAY);
  },

  _stopEventLoop: function() {
    this._clearTimeout(this.eventLoopTimer);
  },

  
  _setTimeout: function(cb, ms) {
    return setTimeout(cb, ms);
  },
  _clearTimeout: function(t) {
    return clearTimeout(t);
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

  this._onWindowReady = this._onWindowReady.bind(this);
  events.on(this.tabActor, "window-ready", this._onWindowReady);
  this._onWindowDestroyed = this._onWindowDestroyed.bind(this);
  events.on(this.tabActor, "window-destroyed", this._onWindowDestroyed);
}

ReflowObserver.prototype = Heritage.extend(Observable.prototype, {
  QueryInterface: XPCOMUtils.generateQI([Ci.nsIReflowObserver,
    Ci.nsISupportsWeakReference]),

  _onWindowReady: function({window}) {
    if (this.observing) {
      this._startListeners([window]);
    }
  },

  _onWindowDestroyed: function({window}) {
    if (this.observing) {
      this._stopListeners([window]);
    }
  },

  _start: function() {
    this._startListeners(this.tabActor.windows);
  },

  _stop: function() {
    if (this.tabActor.attached && this.tabActor.docShell) {
      
      this._stopListeners(this.tabActor.windows);
    }
  },

  _startListeners: function(windows) {
    for (let window of windows) {
      let docshell = window.QueryInterface(Ci.nsIInterfaceRequestor)
                     .getInterface(Ci.nsIWebNavigation)
                     .QueryInterface(Ci.nsIDocShell);
      docshell.addWeakReflowObserver(this);
    }
  },

  _stopListeners: function(windows) {
    for (let window of windows) {
      
      
      try {
        let docshell = window.QueryInterface(Ci.nsIInterfaceRequestor)
                       .getInterface(Ci.nsIWebNavigation)
                       .QueryInterface(Ci.nsIDocShell);
        docshell.removeWeakReflowObserver(this);
      } catch (e) {}
    }
  },

  reflow: function(start, end) {
    this.notifyCallback(start, end, false);
  },

  reflowInterruptible: function(start, end) {
    this.notifyCallback(start, end, true);
  },

  destroy: function() {
    if (this._isDestroyed) {
      return;
    }
    this._isDestroyed = true;

    events.off(this.tabActor, "window-ready", this._onWindowReady);
    events.off(this.tabActor, "window-destroyed", this._onWindowDestroyed);
    Observable.prototype.destroy.call(this);
  }
});
