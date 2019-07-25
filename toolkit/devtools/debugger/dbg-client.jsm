





"use strict";
const Ci = Components.interfaces;
const Cc = Components.classes;
const Cu = Components.utils;
const Cr = Components.results;

var EXPORTED_SYMBOLS = ["DebuggerTransport",
                        "DebuggerClient",
                        "debuggerSocketConnect"];

Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://gre/modules/NetUtil.jsm");
Cu.import("resource://gre/modules/Services.jsm");

XPCOMUtils.defineLazyServiceGetter(this, "socketTransportService",
                                   "@mozilla.org/network/socket-transport-service;1",
                                   "nsISocketTransportService");

let wantLogging = Services.prefs.getBoolPref("devtools.debugger.log");

function dumpn(str)
{
  if (wantLogging) {
    dump("DBG-CLIENT: " + str + "\n");
  }
}

let loader = Cc["@mozilla.org/moz/jssubscript-loader;1"]
  .getService(Ci.mozIJSSubScriptLoader);
loader.loadSubScript("chrome://global/content/devtools/dbg-transport.js");










function eventSource(aProto) {
  









  aProto.addListener = function EV_addListener(aName, aListener) {
    if (typeof aListener != "function") {
      return;
    }

    if (!this._listeners) {
      this._listeners = {};
    }

    if (!aName) {
      aName = '*';
    }

    this._getListeners(aName).push(aListener);
  };

  









  aProto.addOneTimeListener = function EV_addOneTimeListener(aName, aListener) {
    let self = this;

    let l = function() {
      self.removeListener(aName, l);
      aListener.apply(null, arguments);
    };
    this.addListener(aName, l);
  };

  









  aProto.removeListener = function EV_removeListener(aName, aListener) {
    if (!this._listeners || !this._listeners[aName]) {
      return;
    }
    this._listeners[aName] =
      this._listeners[aName].filter(function(l) { return l != aListener });
  };

  






  aProto._getListeners = function EV_getListeners(aName) {
    if (aName in this._listeners) {
      return this._listeners[aName];
    }
    this._listeners[aName] = [];
    return this._listeners[aName];
  };

  








  aProto.notify = function EV_notify() {
    if (!this._listeners) {
      return;
    }

    let name = arguments[0];
    let listeners = this._getListeners(name).slice(0);
    if (this._listeners['*']) {
      listeners.concat(this._listeners['*']);
    }

    for each (let listener in listeners) {
      try {
        listener.apply(null, arguments);
      } catch (e) {
        
        let msg = e + ": " + e.stack;
        Cu.reportError(msg);
        dumpn(msg);
      }
    }
  }
}





const ThreadStateTypes = {
  "paused": "paused",
  "resumed": "attached",
  "detached": "detached"
};





const UnsolicitedNotifications = {
  "newScript": "newScript",
  "tabDetached": "tabDetached",
  "tabNavigated": "tabNavigated"
};





const DebugProtocolTypes = {
  "assign": "assign",
  "attach": "attach",
  "clientEvaluate": "clientEvaluate",
  "delete": "delete",
  "detach": "detach",
  "frames": "frames",
  "interrupt": "interrupt",
  "listTabs": "listTabs",
  "nameAndParameters": "nameAndParameters",
  "ownPropertyNames": "ownPropertyNames",
  "property": "property",
  "prototype": "prototype",
  "prototypeAndProperties": "prototypeAndProperties",
  "resume": "resume",
  "scripts": "scripts",
  "setBreakpoint": "setBreakpoint"
};

const ROOT_ACTOR_NAME = "root";






function DebuggerClient(aTransport)
{
  this._transport = aTransport;
  this._transport.hooks = this;
  this._threadClients = {};
  this._tabClients = {};

  this._pendingRequests = [];
  this._activeRequests = {};
  this._eventsEnabled = true;
}

DebuggerClient.prototype = {
  






  connect: function DC_connect(aOnConnected) {
    if (aOnConnected) {
      this.addOneTimeListener("connected", function(aName, aApplicationType, aTraits) {
        aOnConnected(aApplicationType, aTraits);
      });
    }

    this._transport.ready();
  },

  






  close: function DC_close(aOnClosed) {
    
    
    this._eventsEnabled = false;

    if (aOnClosed) {
      this.addOneTimeListener('closed', function(aEvent) {
        aOnClosed();
      });
    }

    let closeTransport = function _closeTransport() {
      this._transport.close();
      this._transport = null;
    }.bind(this);

    let detachTab = function _detachTab() {
      if (this.activeTab) {
        this.activeTab.detach(closeTransport);
      } else {
        closeTransport();
      }
    }.bind(this);

    if (this.activeThread) {
      this.activeThread.detach(detachTab);
    } else {
      detachTab();
    }
  },

  





  listTabs: function DC_listTabs(aOnResponse) {
    let packet = { to: ROOT_ACTOR_NAME, type: DebugProtocolTypes.listTabs };
    this.request(packet, function(aResponse) {
      aOnResponse(aResponse);
    });
  },

  








  attachTab: function DC_attachTab(aTabActor, aOnResponse) {
    let self = this;
    let packet = { to: aTabActor, type: DebugProtocolTypes.attach };
    this.request(packet, function(aResponse) {
      if (!aResponse.error) {
        var tabClient = new TabClient(self, aTabActor);
        self._tabClients[aTabActor] = tabClient;
        self.activeTab = tabClient;
      }
      aOnResponse(aResponse, tabClient);
    });
  },

  








  attachThread: function DC_attachThread(aThreadActor, aOnResponse) {
    let self = this;
    let packet = { to: aThreadActor, type: DebugProtocolTypes.attach };
    this.request(packet, function(aResponse) {
      if (!aResponse.error) {
        var threadClient = new ThreadClient(self, aThreadActor);
        self._threadClients[aThreadActor] = threadClient;
        self.activeThread = threadClient;
      }
      aOnResponse(aResponse, threadClient);
    });
  },

  








  request: function DC_request(aRequest, aOnResponse) {
    if (!this._connected) {
      throw Error("Have not yet received a hello packet from the server.");
    }
    if (!aRequest.to) {
      let type = aRequest.type || "";
      throw Error("'" + type + "' request packet has no destination.");
    }

    this._pendingRequests.push({ to: aRequest.to,
                                 request: aRequest,
                                 onResponse: aOnResponse });
    this._sendRequests();
  },

  



  _sendRequests: function DC_sendRequests() {
    let self = this;
    this._pendingRequests = this._pendingRequests.filter(function(request) {
      if (request.to in self._activeRequests) {
        return true;
      }

      self._activeRequests[request.to] = request;
      self._transport.send(request.request);

      return false;
    });
  },

  

  





  onPacket: function DC_onPacket(aPacket) {
    if (!this._connected) {
      
      this._connected = true;
      this.notify("connected",
                  aPacket.applicationType,
                  aPacket.traits);
      return;
    }

    try {
      if (!aPacket.from) {
        Cu.reportError("Server did not specify an actor, dropping packet: " +
                       JSON.stringify(aPacket));
        return;
      }

      let onResponse;
      
      if (aPacket.from in this._activeRequests &&
          !(aPacket.type in UnsolicitedNotifications)) {
        onResponse = this._activeRequests[aPacket.from].onResponse;
        delete this._activeRequests[aPacket.from];
      }

      
      if (aPacket.type in ThreadStateTypes &&
          aPacket.from in this._threadClients) {
        this._threadClients[aPacket.from]._onThreadState(aPacket);
      }
      this.notify(aPacket.type, aPacket);

      if (onResponse) {
        onResponse(aPacket);
      }
    } catch(ex) {
      dumpn("Error handling response: " + ex + " - stack:\n" + ex.stack);
      Cu.reportError(ex);
    }

    this._sendRequests();
  },

  






  onClosed: function DC_onClosed(aStatus) {
    this.notify("closed");
  },
}

eventSource(DebuggerClient.prototype);











function TabClient(aClient, aActor) {
  this._client = aClient;
  this._actor = aActor;
}

TabClient.prototype = {
  





  detach: function TabC_detach(aOnResponse) {
    let self = this;
    let packet = { to: this._actor, type: DebugProtocolTypes.detach };
    this._client.request(packet, function(aResponse) {
      if (self.activeTab === self._client._tabClients[self._actor]) {
        delete self.activeTab;
      }
      delete self._client._tabClients[self._actor];
      if (aOnResponse) {
        aOnResponse(aResponse);
      }
    });
  }
};

eventSource(TabClient.prototype);











function ThreadClient(aClient, aActor) {
  this._client = aClient;
  this._actor = aActor;
  this._frameCache = [];
  this._scriptCache = {};
}

ThreadClient.prototype = {
  _state: "paused",
  get state() { return this._state; },
  get paused() { return this._state === "paused"; },

  _pauseOnExceptions: false,

  _actor: null,
  get actor() { return this._actor; },

  _assertPaused: function TC_assertPaused(aCommand) {
    if (!this.paused) {
      throw Error(aCommand + " command sent while not paused.");
    }
  },

  









  resume: function TC_resume(aOnResponse, aLimit) {
    this._assertPaused("resume");

    
    
    this._state = "resuming";

    let self = this;
    let packet = {
      to: this._actor,
      type: DebugProtocolTypes.resume,
      resumeLimit: aLimit,
      pauseOnExceptions: this._pauseOnExceptions
    };
    this._client.request(packet, function(aResponse) {
      if (aResponse.error) {
        
        self._state = "paused";
      }
      if (aOnResponse) {
        aOnResponse(aResponse);
      }
    });
  },

  





  stepOver: function TC_stepOver(aOnResponse) {
    this.resume(aOnResponse, { type: "next" });
  },

  





  stepIn: function TC_stepIn(aOnResponse) {
    this.resume(aOnResponse, { type: "step" });
  },

  





  stepOut: function TC_stepOut(aOnResponse) {
    this.resume(aOnResponse, { type: "finish" });
  },

  





  interrupt: function TC_interrupt(aOnResponse) {
    let packet = { to: this._actor, type: DebugProtocolTypes.interrupt };
    this._client.request(packet, function(aResponse) {
      if (aOnResponse) {
        aOnResponse(aResponse);
      }
    });
  },

  







  pauseOnExceptions: function TC_pauseOnExceptions(aFlag, aOnResponse) {
    this._pauseOnExceptions = aFlag;
    
    
    
    if (!this.paused) {
      this.interrupt(function(aResponse) {
        if (aResponse.error) {
          
          aOnResponse(aResponse);
          return;
        }
        this.resume(aOnResponse);
      }.bind(this));
    }
  },

  











  eval: function TC_eval(aFrame, aExpression, aOnResponse) {
    this._assertPaused("eval");

    
    
    this._state = "resuming";

    let self = this;
    let request = { to: this._actor, type: DebugProtocolTypes.clientEvaluate,
                    frame: aFrame, expression: aExpression };
    this._client.request(request, function(aResponse) {
      if (aResponse.error) {
        
        self._state = "paused";
      }

      if (aOnResponse) {
        aOnResponse(aResponse);
      }
    });
  },

  





  detach: function TC_detach(aOnResponse) {
    let self = this;
    let packet = { to: this._actor, type: DebugProtocolTypes.detach };
    this._client.request(packet, function(aResponse) {
      if (self.activeThread === self._client._threadClients[self._actor]) {
        delete self.activeThread;
      }
      delete self._client._threadClients[self._actor];
      if (aOnResponse) {
        aOnResponse(aResponse);
      }
    });
  },

  







  setBreakpoint: function TC_setBreakpoint(aLocation, aOnResponse) {
    
    let doSetBreakpoint = function _doSetBreakpoint(aCallback) {
      let packet = { to: this._actor, type: DebugProtocolTypes.setBreakpoint,
                     location: aLocation };
      this._client.request(packet, function (aResponse) {
        
        
        if (aOnResponse) {
          let bpClient = new BreakpointClient(this._client, aResponse.actor,
                                              aLocation);
          if (aCallback) {
            aCallback(aOnResponse(aResponse, bpClient));
          } else {
            aOnResponse(aResponse, bpClient);
          }
        }
      }.bind(this));
    }.bind(this);

    
    if (this.paused) {
      doSetBreakpoint();
      return;
    }
    
    this.interrupt(function(aResponse) {
      if (aResponse.error) {
        
        aOnResponse(aResponse);
        return;
      }
      doSetBreakpoint(this.resume.bind(this));
    }.bind(this));
  },

  





  getScripts: function TC_getScripts(aOnResponse) {
    let packet = { to: this._actor, type: DebugProtocolTypes.scripts };
    this._client.request(packet, aOnResponse);
  },

  




  get cachedScripts() { return this._scriptCache; },

  






  fillScripts: function TC_fillScripts() {
    let self = this;
    this.getScripts(function(aResponse) {
      for each (let script in aResponse.scripts) {
        self._scriptCache[script.url] = script;
      }
      
      if (aResponse.scripts && aResponse.scripts.length) {
        self.notify("scriptsadded");
      }
    });
    return true;
  },

  



  _clearScripts: function TC_clearScripts() {
    if (Object.keys(this._scriptCache).length > 0) {
      this._scriptCache = {}
      this.notify("scriptscleared");
    }
  },

  











  getFrames: function TC_getFrames(aStart, aCount, aOnResponse) {
    this._assertPaused("frames");

    let packet = { to: this._actor, type: DebugProtocolTypes.frames,
                   start: aStart, count: aCount ? aCount : undefined };
    this._client.request(packet, aOnResponse);
  },

  




  get cachedFrames() { return this._frameCache; },

  


  get moreFrames() {
    return this.paused && (!this._frameCache || this._frameCache.length == 0
          || !this._frameCache[this._frameCache.length - 1].oldest);
  },

  









  fillFrames: function TC_fillFrames(aTotal) {
    this._assertPaused("fillFrames");

    if (this._frameCache.length >= aTotal) {
      return false;
    }

    let numFrames = this._frameCache.length;

    let self = this;
    this.getFrames(numFrames, aTotal - numFrames, function(aResponse) {
      for each (let frame in aResponse.frames) {
        self._frameCache[frame.depth] = frame;
      }
      
      

      self.notify("framesadded");
    });
    return true;
  },

  



  _clearFrames: function TC_clearFrames() {
    if (this._frameCache.length > 0) {
      this._frameCache = [];
      this.notify("framescleared");
    }
  },

  





  pauseGrip: function TC_pauseGrip(aGrip) {
    if (!this._pauseGrips) {
      this._pauseGrips = {};
    }

    if (aGrip.actor in this._pauseGrips) {
      return this._pauseGrips[aGrip.actor];
    }

    let client = new GripClient(this._client, aGrip);
    this._pauseGrips[aGrip.actor] = client;
    return client;
  },

  



  _clearPauseGrips: function TC_clearPauseGrips(aPacket) {
    for each (let grip in this._pauseGrips) {
      grip.valid = false;
    }
    this._pauseGrips = null;
  },

  



  _onThreadState: function TC_onThreadState(aPacket) {
    this._state = ThreadStateTypes[aPacket.type];
    this._clearFrames();
    this._clearPauseGrips();
    this._client._eventsEnabled && this.notify(aPacket.type, aPacket);
  },
};

eventSource(ThreadClient.prototype);









function GripClient(aClient, aGrip)
{
  this._grip = aGrip;
  this._client = aClient;
}

GripClient.prototype = {
  get actor() { return this._grip.actor },

  _valid: true,
  get valid() { return this._valid; },
  set valid(aValid) { this._valid = !!aValid; },

  





  getSignature: function GC_getSignature(aOnResponse) {
    if (this._grip["class"] !== "Function") {
      throw "getSignature is only valid for function grips.";
    }

    let packet = { to: this.actor, type: DebugProtocolTypes.nameAndParameters };
    this._client.request(packet, function (aResponse) {
                                   if (aOnResponse) {
                                     aOnResponse(aResponse);
                                   }
                                 });
  },

  





  getOwnPropertyNames: function GC_getOwnPropertyNames(aOnResponse) {
    let packet = { to: this.actor, type: DebugProtocolTypes.ownPropertyNames };
    this._client.request(packet, function (aResponse) {
                                   if (aOnResponse) {
                                     aOnResponse(aResponse);
                                   }
                                 });
  },

  




  getPrototypeAndProperties: function GC_getPrototypeAndProperties(aOnResponse) {
    let packet = { to: this.actor,
                   type: DebugProtocolTypes.prototypeAndProperties };
    this._client.request(packet, function (aResponse) {
                                   if (aOnResponse) {
                                     aOnResponse(aResponse);
                                   }
                                 });
  },

  





  getProperty: function GC_getProperty(aName, aOnResponse) {
    let packet = { to: this.actor, type: DebugProtocolTypes.property,
                   name: aName };
    this._client.request(packet, function (aResponse) {
                                   if (aOnResponse) {
                                     aOnResponse(aResponse);
                                   }
                                 });
  },

  




  getPrototype: function GC_getPrototype(aOnResponse) {
    let packet = { to: this.actor, type: DebugProtocolTypes.prototype };
    this._client.request(packet, function (aResponse) {
                                   if (aOnResponse) {
                                     aOnResponse(aResponse);
                                   }
                                 });
  }
};












function BreakpointClient(aClient, aActor, aLocation) {
  this._client = aClient;
  this._actor = aActor;
  this.location = aLocation;
}

BreakpointClient.prototype = {

  _actor: null,
  get actor() { return this._actor; },

  


  remove: function BC_remove(aOnResponse) {
    let packet = { to: this._actor, type: DebugProtocolTypes["delete"] };
    this._client.request(packet, function(aResponse) {
                                   if (aOnResponse) {
                                     aOnResponse(aResponse);
                                   }
                                 });
  }
};

eventSource(BreakpointClient.prototype);









function debuggerSocketConnect(aHost, aPort)
{
  let s = socketTransportService.createTransport(null, 0, aHost, aPort, null);
  let transport = new DebuggerTransport(s.openInputStream(0, 0, 0),
                                        s.openOutputStream(0, 0, 0));
  return transport;
}
