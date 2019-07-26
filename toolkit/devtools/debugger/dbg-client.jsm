





"use strict";
const Ci = Components.interfaces;
const Cc = Components.classes;
const Cu = Components.utils;
const Cr = Components.results;

this.EXPORTED_SYMBOLS = ["DebuggerTransport",
                         "DebuggerClient",
                         "debuggerSocketConnect",
                         "LongStringClient"];

Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://gre/modules/NetUtil.jsm");
Cu.import("resource://gre/modules/Services.jsm");

XPCOMUtils.defineLazyServiceGetter(this, "socketTransportService",
                                   "@mozilla.org/network/socket-transport-service;1",
                                   "nsISocketTransportService");

XPCOMUtils.defineLazyModuleGetter(this, "WebConsoleClient",
                                  "resource://gre/modules/devtools/WebConsoleClient.jsm");

let wantLogging = Services.prefs.getBoolPref("devtools.debugger.log");

function dumpn(str)
{
  if (wantLogging) {
    dump("DBG-CLIENT: " + str + "\n");
  }
}

let loader = Cc["@mozilla.org/moz/jssubscript-loader;1"]
  .getService(Ci.mozIJSSubScriptLoader);
loader.loadSubScript("chrome://global/content/devtools/dbg-transport.js", this);










function eventSource(aProto) {
  









  aProto.addListener = function EV_addListener(aName, aListener) {
    if (typeof aListener != "function") {
      throw TypeError("Listeners must be functions.");
    }

    if (!this._listeners) {
      this._listeners = {};
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
  "consoleAPICall": "consoleAPICall",
  "eventNotification": "eventNotification",
  "fileActivity": "fileActivity",
  "locationChange": "locationChange",
  "networkEvent": "networkEvent",
  "networkEventUpdate": "networkEventUpdate",
  "newGlobal": "newGlobal",
  "newScript": "newScript",
  "tabDetached": "tabDetached",
  "tabNavigated": "tabNavigated",
  "pageError": "pageError"
};





const UnsolicitedPauses = {
  "resumeLimit": "resumeLimit",
  "debuggerStatement": "debuggerStatement",
  "breakpoint": "breakpoint",
  "watchpoint": "watchpoint"
};

const ROOT_ACTOR_NAME = "root";






this.DebuggerClient = function DebuggerClient(aTransport)
{
  this._transport = aTransport;
  this._transport.hooks = this;
  this._threadClients = {};
  this._tabClients = {};
  this._consoleClients = {};

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

    let detachThread = function _detachThread() {
      if (this.activeThread) {
        this.activeThread.detach(detachTab);
      } else {
        detachTab();
      }
    }.bind(this);

    let consolesClosed = 0;
    let consolesToClose = 0;

    let onConsoleClose = function _onConsoleClose() {
      consolesClosed++;
      if (consolesClosed >= consolesToClose) {
        this._consoleClients = {};
        detachThread();
      }
    }.bind(this);

    for each (let client in this._consoleClients) {
      consolesToClose++;
      client.close(onConsoleClose);
    }

    if (!consolesToClose) {
      detachThread();
    }
  },

  





  listTabs: function DC_listTabs(aOnResponse) {
    let packet = { to: ROOT_ACTOR_NAME, type: "listTabs" };
    this.request(packet, function(aResponse) {
      aOnResponse(aResponse);
    });
  },

  








  attachTab: function DC_attachTab(aTabActor, aOnResponse) {
    let self = this;
    let packet = { to: aTabActor, type: "attach" };
    this.request(packet, function(aResponse) {
      let tabClient;
      if (!aResponse.error) {
        tabClient = new TabClient(self, aTabActor);
        self._tabClients[aTabActor] = tabClient;
        self.activeTab = tabClient;
      }
      aOnResponse(aResponse, tabClient);
    });
  },

  










  attachConsole:
  function DC_attachConsole(aConsoleActor, aListeners, aOnResponse) {
    let self = this;
    let packet = {
      to: aConsoleActor,
      type: "startListeners",
      listeners: aListeners,
    };

    this.request(packet, function(aResponse) {
      let consoleClient;
      if (!aResponse.error) {
        consoleClient = new WebConsoleClient(self, aConsoleActor);
        self._consoleClients[aConsoleActor] = consoleClient;
      }
      aOnResponse(aResponse, consoleClient);
    });
  },

  








  attachThread: function DC_attachThread(aThreadActor, aOnResponse) {
    let self = this;
    let packet = { to: aThreadActor, type: "attach" };
    this.request(packet, function(aResponse) {
      if (!aResponse.error) {
        var threadClient = new ThreadClient(self, aThreadActor);
        self._threadClients[aThreadActor] = threadClient;
        self.activeThread = threadClient;
      }
      aOnResponse(aResponse, threadClient);
    });
  },

  








  release: function DC_release(aActor, aOnResponse) {
    let packet = {
      to: aActor,
      type: "release",
    };
    this.request(packet, aOnResponse);
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
        let msg = "Server did not specify an actor, dropping packet: " +
                  JSON.stringify(aPacket);
        Cu.reportError(msg);
        dumpn(msg);
        return;
      }

      let onResponse;
      
      if (aPacket.from in this._activeRequests &&
          !(aPacket.type in UnsolicitedNotifications) &&
          !(aPacket.type == ThreadStateTypes.paused &&
            aPacket.why.type in UnsolicitedPauses)) {
        onResponse = this._activeRequests[aPacket.from].onResponse;
        delete this._activeRequests[aPacket.from];
      }

      
      if (aPacket.type in ThreadStateTypes &&
          aPacket.from in this._threadClients) {
        this._threadClients[aPacket.from]._onThreadState(aPacket);
      }
      
      
      
      if (this.activeThread &&
          aPacket.type == UnsolicitedNotifications.tabNavigated &&
          aPacket.from in this._tabClients) {
        let resumption = { from: this.activeThread._actor, type: "resumed" };
        this.activeThread._onThreadState(resumption);
      }
      this.notify(aPacket.type, aPacket);

      if (onResponse) {
        onResponse(aPacket);
      }
    } catch(ex) {
      dumpn("Error handling response: " + ex + " - stack:\n" + ex.stack);
      Cu.reportError(ex.message + "\n" + ex.stack);
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
    let packet = { to: this._actor, type: "detach" };
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
  this._pauseGrips = {};
  this._threadGrips = {};
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
      type: "resume",
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
    let packet = { to: this._actor, type: "interrupt" };
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
    let request = { to: this._actor, type: "clientEvaluate",
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
    let packet = { to: this._actor, type: "detach" };
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
      let packet = { to: this._actor, type: "setBreakpoint",
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

  







  releaseMany: function TC_releaseMany(aActors, aOnResponse) {
    let packet = {
      to: this._actor,
      type: "releaseMany",
      actors: aActors
    };
    this._client.request(packet, aOnResponse);
  },

  





  threadGrips: function TC_threadGrips(aActors, aOnResponse) {
    let packet = {
      to: this._actor,
      type: "threadGrips",
      actors: aActors
    };
    this._client.request(packet, aOnResponse);
  },

  





  getScripts: function TC_getScripts(aOnResponse) {
    let packet = { to: this._actor, type: "scripts" };
    this._client.request(packet, aOnResponse);
  },

  _doInterrupted: function TC_doInterrupted(aAction, aError) {
    if (this.paused) {
      aAction();
      return;
    }
    this.interrupt(function(aResponse) {
      if (aResponse) {
        aError(aResponse);
        return;
      }
      aAction();
      this.resume(function() {});
    }.bind(this));
  },

  






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

    let packet = { to: this._actor, type: "frames",
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
    if (aGrip.actor in this._pauseGrips) {
      return this._pauseGrips[aGrip.actor];
    }

    let client = new GripClient(this._client, aGrip);
    this._pauseGrips[aGrip.actor] = client;
    return client;
  },

  









  _longString: function TC__longString(aGrip, aGripCacheName) {
    if (aGrip.actor in this[aGripCacheName]) {
      return this[aGripCacheName][aGrip.actor];
    }

    let client = new LongStringClient(this._client, aGrip);
    this[aGripCacheName][aGrip.actor] = client;
    return client;
  },

  






  pauseLongString: function TC_pauseLongString(aGrip) {
    return this._longString(aGrip, "_pauseGrips");
  },

  






  threadLongString: function TC_threadLongString(aGrip) {
    return this._longString(aGrip, "_threadGrips");
  },

  





  _clearGripClients: function TC_clearGrips(aGripCacheName) {
    for each (let grip in this[aGripCacheName]) {
      grip.valid = false;
    }
    this[aGripCacheName] = {};
  },

  



  _clearPauseGrips: function TC_clearPauseGrips() {
    this._clearGripClients("_pauseGrips");
  },

  



  _clearThreadGrips: function TC_clearPauseGrips() {
    this._clearGripClients("_threadGrips");
  },

  



  _onThreadState: function TC_onThreadState(aPacket) {
    this._state = ThreadStateTypes[aPacket.type];
    this._clearFrames();
    this._clearPauseGrips();
    aPacket.type === ThreadStateTypes.detached && this._clearThreadGrips();
    this._client._eventsEnabled && this.notify(aPacket.type, aPacket);
  },

  


  source: function TC_source(aActor) {
    return new SourceClient(this._client, aActor);
  }

};

eventSource(ThreadClient.prototype);









function GripClient(aClient, aGrip)
{
  this._grip = aGrip;
  this._client = aClient;
}

GripClient.prototype = {
  get actor() { return this._grip.actor },

  valid: true,

  







  getParameterNames: function GC_getParameterNames(aOnResponse) {
    if (this._grip["class"] !== "Function") {
      throw "getParameterNames is only valid for function grips.";
    }

    let packet = { to: this.actor, type: "parameterNames" };
    this._client.request(packet, function (aResponse) {
                                   if (aOnResponse) {
                                     aOnResponse(aResponse);
                                   }
                                 });
  },

  





  getOwnPropertyNames: function GC_getOwnPropertyNames(aOnResponse) {
    let packet = { to: this.actor, type: "ownPropertyNames" };
    this._client.request(packet, function (aResponse) {
                                   if (aOnResponse) {
                                     aOnResponse(aResponse);
                                   }
                                 });
  },

  




  getPrototypeAndProperties: function GC_getPrototypeAndProperties(aOnResponse) {
    let packet = { to: this.actor,
                   type: "prototypeAndProperties" };
    this._client.request(packet, function (aResponse) {
                                   if (aOnResponse) {
                                     aOnResponse(aResponse);
                                   }
                                 });
  },

  





  getProperty: function GC_getProperty(aName, aOnResponse) {
    let packet = { to: this.actor, type: "property",
                   name: aName };
    this._client.request(packet, function (aResponse) {
                                   if (aOnResponse) {
                                     aOnResponse(aResponse);
                                   }
                                 });
  },

  




  getPrototype: function GC_getPrototype(aOnResponse) {
    let packet = { to: this.actor, type: "prototype" };
    this._client.request(packet, function (aResponse) {
                                   if (aOnResponse) {
                                     aOnResponse(aResponse);
                                   }
                                 });
  }
};










function LongStringClient(aClient, aGrip) {
  this._grip = aGrip;
  this._client = aClient;
}

LongStringClient.prototype = {
  get actor() { return this._grip.actor; },
  get length() { return this._grip.length; },
  get initial() { return this._grip.initial; },

  valid: true,

  









  substring: function LSC_substring(aStart, aEnd, aCallback) {
    let packet = { to: this.actor,
                   type: "substring",
                   start: aStart,
                   end: aEnd };
    this._client.request(packet, aCallback);
  }
};









function SourceClient(aClient, aActor) {
  this._actor = aActor;
  this._client = aClient;
}

SourceClient.prototype = {
  


  source: function SC_source(aCallback) {
    let packet = {
      to: this._actor,
      type: "source"
    };
    this._client.request(packet, function (aResponse) {
      if (aResponse.error) {
        aCallback(aResponse);
        return;
      }

      if (typeof aResponse.source === "string") {
        aCallback(aResponse);
        return;
      }

      let longString = this._client.activeThread.threadLongString(
        aResponse.source);
      longString.substring(0, longString.length, function (aResponse) {
        if (aResponse.error) {
          aCallback(aResponse);
          return;
        }

        aCallback({
          source: aResponse.substring
        });
      });
    }.bind(this));
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
    let packet = { to: this._actor, type: "delete" };
    this._client.request(packet, function(aResponse) {
                                   if (aOnResponse) {
                                     aOnResponse(aResponse);
                                   }
                                 });
  }
};

eventSource(BreakpointClient.prototype);









this.debuggerSocketConnect = function debuggerSocketConnect(aHost, aPort)
{
  let s = socketTransportService.createTransport(null, 0, aHost, aPort, null);
  let transport = new DebuggerTransport(s.openInputStream(0, 0, 0),
                                        s.openOutputStream(0, 0, 0));
  return transport;
}
