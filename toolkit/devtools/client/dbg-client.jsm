





"use strict";
const Ci = Components.interfaces;
const Cc = Components.classes;
const Cu = Components.utils;
const Cr = Components.results;

this.EXPORTED_SYMBOLS = ["DebuggerTransport",
                         "DebuggerClient",
                         "RootClient",
                         "debuggerSocketConnect",
                         "LongStringClient",
                         "EnvironmentClient",
                         "ObjectClient"];

Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://gre/modules/NetUtil.jsm");
Cu.import("resource://gre/modules/Services.jsm");
Cu.import("resource://gre/modules/Timer.jsm");
let promise = Cu.import("resource://gre/modules/commonjs/sdk/core/promise.js").Promise;
const { defer, resolve, reject } = promise;

XPCOMUtils.defineLazyServiceGetter(this, "socketTransportService",
                                   "@mozilla.org/network/socket-transport-service;1",
                                   "nsISocketTransportService");

XPCOMUtils.defineLazyModuleGetter(this, "devtools",
                                  "resource://gre/modules/devtools/Loader.jsm");

Object.defineProperty(this, "WebConsoleClient", {
  get: function() {
    return devtools.require("devtools/toolkit/webconsole/client").WebConsoleClient;
  },
  configurable: true,
  enumerable: true
});

Components.utils.import("resource://gre/modules/devtools/DevToolsUtils.jsm");
this.makeInfallible = DevToolsUtils.makeInfallible;

let wantLogging = Services.prefs.getBoolPref("devtools.debugger.log");

function dumpn(str)
{
  if (wantLogging) {
    dump("DBG-CLIENT: " + str + "\n");
  }
}

let loader = Cc["@mozilla.org/moz/jssubscript-loader;1"]
  .getService(Ci.mozIJSSubScriptLoader);
loader.loadSubScript("resource://gre/modules/devtools/server/transport.js", this);










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
    let l = (...args) => {
      this.removeListener(aName, l);
      aListener.apply(null, args);
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
  "lastPrivateContextExited": "lastPrivateContextExited",
  "logMessage": "logMessage",
  "networkEvent": "networkEvent",
  "networkEventUpdate": "networkEventUpdate",
  "newGlobal": "newGlobal",
  "newScript": "newScript",
  "newSource": "newSource",
  "tabDetached": "tabDetached",
  "tabListChanged": "tabListChanged",
  "addonListChanged": "addonListChanged",
  "tabNavigated": "tabNavigated",
  "pageError": "pageError",
  "documentLoad": "documentLoad",
  "enteredFrame": "enteredFrame",
  "exitedFrame": "exitedFrame",
  "appOpen": "appOpen",
  "appClose": "appClose",
  "appInstall": "appInstall",
  "appUninstall": "appUninstall"
};





const UnsolicitedPauses = {
  "resumeLimit": "resumeLimit",
  "debuggerStatement": "debuggerStatement",
  "breakpoint": "breakpoint",
  "DOMEvent": "DOMEvent",
  "watchpoint": "watchpoint",
  "exception": "exception"
};






this.DebuggerClient = function DebuggerClient(aTransport)
{
  this._transport = aTransport;
  this._transport.hooks = this;
  this._threadClients = {};
  this._tabClients = {};
  this._consoleClients = {};

  this._pendingRequests = [];
  this._activeRequests = new Map;
  this._eventsEnabled = true;

  this.compat = new ProtocolCompatibility(this, [
    new SourcesShim(),
  ]);

  this.request = this.request.bind(this);
  this.localTransport = this._transport.onOutputStreamReady === undefined;

  



  this.mainRoot = null;
  this.expectReply("root", (aPacket) => {
    this.mainRoot = new RootClient(this, aPacket);
    this.notify("connected", aPacket.applicationType, aPacket.traits);
  });
}



















DebuggerClient.requester = function DC_requester(aPacketSkeleton, { telemetry,
                                                 before, after }) {
  return function (...args) {
    let histogram, startTime;
    if (telemetry) {
      let transportType = this._transport.onOutputStreamReady === undefined
        ? "LOCAL_"
        : "REMOTE_";
      let histogramId = "DEVTOOLS_DEBUGGER_RDP_"
        + transportType + telemetry + "_MS";
      histogram = Services.telemetry.getHistogramById(histogramId);
      startTime = +new Date;
    }
    let outgoingPacket = {
      to: aPacketSkeleton.to || this.actor
    };

    let maxPosition = -1;
    for (let k of Object.keys(aPacketSkeleton)) {
      if (aPacketSkeleton[k] instanceof DebuggerClient.Argument) {
        let { position } = aPacketSkeleton[k];
        outgoingPacket[k] = aPacketSkeleton[k].getArgument(args);
        maxPosition = Math.max(position, maxPosition);
      } else {
        outgoingPacket[k] = aPacketSkeleton[k];
      }
    }

    if (before) {
      outgoingPacket = before.call(this, outgoingPacket);
    }

    this.request(outgoingPacket, function (aResponse) {
      if (after) {
        let { from } = aResponse;
        aResponse = after.call(this, aResponse);
        if (!aResponse.from) {
          aResponse.from = from;
        }
      }

      
      let thisCallback = args[maxPosition + 1];
      if (thisCallback) {
        try {
          thisCallback(aResponse);
        } catch (e) {
          let msg = "Error executing callback passed to debugger client: "
            + e + "\n" + e.stack;
          dumpn(msg);
          Cu.reportError(msg);
        }
      }

      if (histogram) {
        histogram.add(+new Date - startTime);
      }
    }.bind(this));

  };
};

function args(aPos) {
  return new DebuggerClient.Argument(aPos);
}

DebuggerClient.Argument = function DCP(aPosition) {
  this.position = aPosition;
};

DebuggerClient.Argument.prototype.getArgument = function DCP_getArgument(aParams) {
  if (!(this.position in aParams)) {
    throw new Error("Bad index into params: " + this.position);
  }
  return aParams[this.position];
};

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

    
    
    
    
    let self = this;

    let continuation = function () {
      self._consoleClients = {};
      detachThread();
    }

    for each (let client in this._consoleClients) {
      continuation = client.close.bind(client, continuation);
    }

    continuation();

    function detachThread() {
      if (self.activeThread) {
        self.activeThread.detach(detachTab);
      } else {
        detachTab();
      }
    }

    function detachTab() {
      if (self.activeTab) {
        self.activeTab.detach(closeTransport);
      } else {
        closeTransport();
      }
    }

    function closeTransport() {
      self._transport.close();
      self._transport = null;
    }
  },

  



  listTabs: function(aOnResponse) { return this.mainRoot.listTabs(aOnResponse); },

  



  listAddons: function(aOnResponse) { return this.mainRoot.listAddons(aOnResponse); },

  








  attachTab: function DC_attachTab(aTabActor, aOnResponse) {
    let packet = {
      to: aTabActor,
      type: "attach"
    };
    this.request(packet, (aResponse) => {
      let tabClient;
      if (!aResponse.error) {
        tabClient = new TabClient(this, aTabActor);
        this._tabClients[aTabActor] = tabClient;
        this.activeTab = tabClient;
      }
      aOnResponse(aResponse, tabClient);
    });
  },

  










  attachConsole:
  function DC_attachConsole(aConsoleActor, aListeners, aOnResponse) {
    let packet = {
      to: aConsoleActor,
      type: "startListeners",
      listeners: aListeners,
    };

    this.request(packet, (aResponse) => {
      let consoleClient;
      if (!aResponse.error) {
        consoleClient = new WebConsoleClient(this, aConsoleActor);
        this._consoleClients[aConsoleActor] = consoleClient;
      }
      aOnResponse(aResponse, consoleClient);
    });
  },

  











  attachThread: function DC_attachThread(aThreadActor, aOnResponse, aOptions={}) {
    let packet = {
      to: aThreadActor,
      type: "attach",
      options: aOptions
    };
    this.request(packet, (aResponse) => {
      if (!aResponse.error) {
        var threadClient = new ThreadClient(this, aThreadActor);
        this._threadClients[aThreadActor] = threadClient;
        this.activeThread = threadClient;
      }
      aOnResponse(aResponse, threadClient);
    });
  },

  








  attachTracer: function DC_attachTracer(aTraceActor, aOnResponse) {
    let packet = {
      to: aTraceActor,
      type: "attach"
    };
    this.request(packet, (aResponse) => {
      if (!aResponse.error) {
        let traceClient = new TraceClient(this, aTraceActor);
        aOnResponse(aResponse, traceClient);
      }
    });
  },

  







  reconfigureThread: function(aOptions, aOnResponse) {
    let packet = {
      to: this.activeThread._actor,
      type: "reconfigure",
      options: aOptions
    };
    this.request(packet, aOnResponse);
  },

  








  release: DebuggerClient.requester({
    to: args(0),
    type: "release"
  }, {
    telemetry: "RELEASE"
  }),

  








  request: function DC_request(aRequest, aOnResponse) {
    if (!this.mainRoot) {
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
    this._pendingRequests = this._pendingRequests.filter((request) => {
      if (this._activeRequests.has(request.to)) {
        return true;
      }

      this.expectReply(request.to, request.onResponse);
      this._transport.send(request.request);

      return false;
    });
  },

  







  expectReply: function(aActor, aHandler) {
    if (this._activeRequests.has(aActor)) {
      throw Error("clashing handlers for next reply from " + uneval(aActor));
    }
    this._activeRequests.set(aActor, aHandler);
  },

  

  







  onPacket: function DC_onPacket(aPacket, aIgnoreCompatibility=false) {
    let packet = aIgnoreCompatibility
      ? aPacket
      : this.compat.onPacket(aPacket);

    resolve(packet).then((aPacket) => {
      if (!aPacket.from) {
        let msg = "Server did not specify an actor, dropping packet: " +
                  JSON.stringify(aPacket);
        Cu.reportError(msg);
        dumpn(msg);
        return;
      }

      
      
      let front = this.getActor(aPacket.from);
      if (front) {
        front.onPacket(aPacket);
        return;
      }

      let onResponse;
      
      
      
      if (this._activeRequests.has(aPacket.from) &&
          !(aPacket.type in UnsolicitedNotifications) &&
          !(aPacket.type == ThreadStateTypes.paused &&
            aPacket.why.type in UnsolicitedPauses)) {
        onResponse = this._activeRequests.get(aPacket.from);
        this._activeRequests.delete(aPacket.from);
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
      
      
      if (aPacket.type) {
        this.notify(aPacket.type, aPacket);
      }

      if (onResponse) {
        onResponse(aPacket);
      }

      this._sendRequests();
    }, function (ex) {
      dumpn("Error handling response: " + ex + " - stack:\n" + ex.stack);
      Cu.reportError(ex.message + "\n" + ex.stack);
    });
  },

  






  onClosed: function DC_onClosed(aStatus) {
    this.notify("closed");
  },

  


  __pools: null,
  get _pools() {
    if (this.__pools) {
      return this.__pools;
    }
    this.__pools = new Set();
    return this.__pools;
  },

  addActorPool: function(pool) {
    this._pools.add(pool);
  },
  removeActorPool: function(pool) {
    this._pools.delete(pool);
  },
  getActor: function(actorID) {
    let pool = this.poolFor(actorID);
    return pool ? pool.get(actorID) : null;
  },

  poolFor: function(actorID) {
    for (let pool of this._pools) {
      if (pool.has(actorID)) return pool;
    }
    return null;
  }
}

eventSource(DebuggerClient.prototype);


const SUPPORTED = 1;
const NOT_SUPPORTED = 2;
const SKIP = 3;









function ProtocolCompatibility(aClient, aFeatures) {
  this._client = aClient;
  this._featuresWithUnknownSupport = new Set(aFeatures);
  this._featuresWithoutSupport = new Set();

  this._featureDeferreds = Object.create(null)
  for (let f of aFeatures) {
    this._featureDeferreds[f.name] = defer();
  }
}

ProtocolCompatibility.prototype = {
  






  supportsFeature: function PC_supportsFeature(aFeatureName) {
    return this._featureDeferreds[aFeatureName].promise;
  },

  





  rejectFeature: function PC_rejectFeature(aFeatureName) {
    this._featureDeferreds[aFeatureName].reject(false);
  },

  






  onPacket: function PC_onPacket(aPacket) {
    this._detectFeatures(aPacket);
    return this._shimPacket(aPacket);
  },

  



  _detectFeatures: function PC__detectFeatures(aPacket) {
    for (let feature of this._featuresWithUnknownSupport) {
      try {
        switch (feature.onPacketTest(aPacket)) {
        case SKIP:
          break;
        case SUPPORTED:
          this._featuresWithUnknownSupport.delete(feature);
          this._featureDeferreds[feature.name].resolve(true);
          break;
        case NOT_SUPPORTED:
          this._featuresWithUnknownSupport.delete(feature);
          this._featuresWithoutSupport.add(feature);
          this.rejectFeature(feature.name);
          break;
        default:
          Cu.reportError(new Error(
            "Bad return value from `onPacketTest` for feature '"
              + feature.name + "'"));
        }
      } catch (ex) {
        Cu.reportError("Error detecting support for feature '"
                       + feature.name + "':" + ex.message + "\n"
                       + ex.stack);
      }
    }
  },

  



  _shimPacket: function PC__shimPacket(aPacket) {
    let extraPackets = [];

    let loop = function (aFeatures, aPacket) {
      if (aFeatures.length === 0) {
        for (let packet of extraPackets) {
          this._client.onPacket(packet, true);
        }
        return aPacket;
      } else {
        let replacePacket = function (aNewPacket) {
          return aNewPacket;
        };
        let extraPacket = function (aExtraPacket) {
          extraPackets.push(aExtraPacket);
          return aPacket;
        };
        let keepPacket = function () {
          return aPacket;
        };
        let newPacket = aFeatures[0].translatePacket(aPacket,
                                                     replacePacket,
                                                     extraPacket,
                                                     keepPacket);
        return resolve(newPacket).then(loop.bind(null, aFeatures.slice(1)));
      }
    }.bind(this);

    return loop([f for (f of this._featuresWithoutSupport)],
                aPacket);
  }
};




const FeatureCompatibilityShim = {
  
  name: null,

  



  onPacketTest: function (aPacket) {
    throw new Error("Not yet implemented");
  },

  



  translatePacket: function (aPacket, aReplacePacket, aExtraPacket, aKeepPacket) {
    throw new Error("Not yet implemented");
  }
};





function SourcesShim() {
  this._sourcesSeen = new Set();
}

SourcesShim.prototype = Object.create(FeatureCompatibilityShim);
let SSProto = SourcesShim.prototype;

SSProto.name = "sources";

SSProto.onPacketTest = function SS_onPacketTest(aPacket) {
  if (aPacket.traits) {
    return aPacket.traits.sources
      ? SUPPORTED
      : NOT_SUPPORTED;
  }
  return SKIP;
};

SSProto.translatePacket = function SS_translatePacket(aPacket,
                                                      aReplacePacket,
                                                      aExtraPacket,
                                                      aKeepPacket) {
  if (aPacket.type !== "newScript" || this._sourcesSeen.has(aPacket.url)) {
    return aKeepPacket();
  }
  this._sourcesSeen.add(aPacket.url);
  return aExtraPacket({
    from: aPacket.from,
    type: "newSource",
    source: aPacket.source
  });
};











function TabClient(aClient, aActor) {
  this._client = aClient;
  this._actor = aActor;
  this.request = this._client.request;
}

TabClient.prototype = {
  get actor() { return this._actor },
  get _transport() { return this._client._transport; },

  





  detach: DebuggerClient.requester({
    type: "detach"
  }, {
    after: function (aResponse) {
      if (this.activeTab === this._client._tabClients[this.actor]) {
        delete this.activeTab;
      }
      delete this._client._tabClients[this.actor];
      return aResponse;
    },
    telemetry: "TABDETACH"
  }),

  


  reload: DebuggerClient.requester({
    type: "reload"
  }, {
    telemetry: "RELOAD"
  }),

  





  navigateTo: DebuggerClient.requester({
    type: "navigateTo",
    url: args(0)
  }, {
    telemetry: "NAVIGATETO"
  }),
};

eventSource(TabClient.prototype);





















function RootClient(aClient, aGreeting) {
  this._client = aClient;
  this.actor = aGreeting.from;
  this.applicationType = aGreeting.applicationType;
  this.traits = aGreeting.traits;
}

RootClient.prototype = {
  constructor: RootClient,

  





  listTabs: DebuggerClient.requester({ type: "listTabs" },
                                     { telemetry: "LISTTABS" }),

  





  listAddons: DebuggerClient.requester({ type: "listAddons" },
                                       { telemetry: "LISTADDONS" }),

  



  get _transport() { return this._client._transport; },
  get request()    { return this._client.request;    }
};











function ThreadClient(aClient, aActor) {
  this._client = aClient;
  this._actor = aActor;
  this._frameCache = [];
  this._scriptCache = {};
  this._pauseGrips = {};
  this._threadGrips = {};
  this.request = this._client.request;
}

ThreadClient.prototype = {
  _state: "paused",
  get state() { return this._state; },
  get paused() { return this._state === "paused"; },

  _pauseOnExceptions: false,
  _ignoreCaughtExceptions: false,
  _pauseOnDOMEvents: null,

  _actor: null,
  get actor() { return this._actor; },

  get compat() { return this._client.compat; },
  get _transport() { return this._client._transport; },

  _assertPaused: function TC_assertPaused(aCommand) {
    if (!this.paused) {
      throw Error(aCommand + " command sent while not paused.");
    }
  },

  










  _doResume: DebuggerClient.requester({
    type: "resume",
    resumeLimit: args(0)
  }, {
    before: function (aPacket) {
      this._assertPaused("resume");

      
      
      this._state = "resuming";

      if (!aPacket.resumeLimit) {
        delete aPacket.resumeLimit;
      }
      if (this._pauseOnExceptions) {
        aPacket.pauseOnExceptions = this._pauseOnExceptions;
      }
      if (this._ignoreCaughtExceptions) {
        aPacket.ignoreCaughtExceptions = this._ignoreCaughtExceptions;
      }
      if (this._pauseOnDOMEvents) {
        aPacket.pauseOnDOMEvents = this._pauseOnDOMEvents;
      }
      return aPacket;
    },
    after: function (aResponse) {
      if (aResponse.error) {
        
        this._state = "paused";
      }
      return aResponse;
    },
    telemetry: "RESUME"
  }),

  


  resume: function TC_resume(aOnResponse) {
    this._doResume(null, aOnResponse);
  },

  





  stepOver: function TC_stepOver(aOnResponse) {
    this._doResume({ type: "next" }, aOnResponse);
  },

  





  stepIn: function TC_stepIn(aOnResponse) {
    this._doResume({ type: "step" }, aOnResponse);
  },

  





  stepOut: function TC_stepOut(aOnResponse) {
    this._doResume({ type: "finish" }, aOnResponse);
  },

  





  interrupt: DebuggerClient.requester({
    type: "interrupt"
  }, {
    telemetry: "INTERRUPT"
  }),

  







  pauseOnExceptions: function TC_pauseOnExceptions(aPauseOnExceptions,
                                                   aIgnoreCaughtExceptions,
                                                   aOnResponse) {
    this._pauseOnExceptions = aPauseOnExceptions;
    this._ignoreCaughtExceptions = aIgnoreCaughtExceptions;

    
    
    if (this.paused) {
      this._client.reconfigureThread({
        pauseOnExceptions: aPauseOnExceptions,
        ignoreCaughtExceptions: aIgnoreCaughtExceptions
      }, aOnResponse);
      return;
    }
    
    this.interrupt(aResponse => {
      if (aResponse.error) {
        
        aOnResponse(aResponse);
        return;
      }
      this.resume(aOnResponse);
    });
  },

  











  pauseOnDOMEvents: function (events, onResponse) {
    this._pauseOnDOMEvents = events;
    
    
    
    if (this.paused)
      return void setTimeout(onResponse, 0);
    this.interrupt(response => {
      
      if (response.error)
        return void onResponse(response);
      this.resume(onResponse);
    });
  },

  











  eval: DebuggerClient.requester({
    type: "clientEvaluate",
    frame: args(0),
    expression: args(1)
  }, {
    before: function (aPacket) {
      this._assertPaused("eval");
      
      
      this._state = "resuming";
      return aPacket;
    },
    after: function (aResponse) {
      if (aResponse.error) {
        
        self._state = "paused";
      }
      return aResponse;
    },
    telemetry: "CLIENTEVALUATE"
  }),

  





  detach: DebuggerClient.requester({
    type: "detach"
  }, {
    after: function (aResponse) {
      if (this.activeThread === this._client._threadClients[this.actor]) {
        delete this.activeThread;
      }
      delete this._client._threadClients[this.actor];
      return aResponse;
    },
    telemetry: "THREADDETACH"
  }),

  







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

  







  releaseMany: DebuggerClient.requester({
    type: "releaseMany",
    actors: args(0),
  }, {
    telemetry: "RELEASEMANY"
  }),

  





  threadGrips: DebuggerClient.requester({
    type: "threadGrips",
    actors: args(0)
  }, {
    telemetry: "THREADGRIPS"
  }),

  





  eventListeners: DebuggerClient.requester({
    type: "eventListeners"
  }, {
    telemetry: "EVENTLISTENERS"
  }),

  





  getSources: function TC_getSources(aOnResponse) {
    
    
    let getSources = DebuggerClient.requester({
      type: "sources"
    }, {
      telemetry: "SOURCES"
    });

    
    
    let getSourcesBackwardsCompat = DebuggerClient.requester({
      type: "scripts"
    }, {
      after: function (aResponse) {
        if (aResponse.error) {
          return aResponse;
        }

        let sourceActorsByURL = aResponse.scripts
          .reduce(function (aSourceActorsByURL, aScript) {
            aSourceActorsByURL[aScript.url] = aScript.source;
            return aSourceActorsByURL;
          }, {});

        return {
          sources: [
            { url: url, actor: sourceActorsByURL[url] }
            for (url of Object.keys(sourceActorsByURL))
          ]
        }
      },
      telemetry: "SOURCES"
    });

    
    
    let threadClient = this;
    this.compat.supportsFeature("sources").then(function () {
      threadClient.getSources = getSources;
    }, function () {
      threadClient.getSources = getSourcesBackwardsCompat;
    }).then(function () {
      threadClient.getSources(aOnResponse);
    });
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
      this.resume();
    }.bind(this));
  },

  



  _clearScripts: function TC_clearScripts() {
    if (Object.keys(this._scriptCache).length > 0) {
      this._scriptCache = {}
      this.notify("scriptscleared");
    }
  },

  











  getFrames: DebuggerClient.requester({
    type: "frames",
    start: args(0),
    count: args(1)
  }, {
    telemetry: "FRAMES"
  }),

  




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

    this.getFrames(numFrames, aTotal - numFrames, (aResponse) => {
      for each (let frame in aResponse.frames) {
        this._frameCache[frame.depth] = frame;
      }
      
      

      this.notify("framesadded");
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

    let client = new ObjectClient(this._client, aGrip);
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

  





  _clearObjectClients: function TC_clearGrips(aGripCacheName) {
    for each (let grip in this[aGripCacheName]) {
      grip.valid = false;
    }
    this[aGripCacheName] = {};
  },

  



  _clearPauseGrips: function TC_clearPauseGrips() {
    this._clearObjectClients("_pauseGrips");
  },

  



  _clearThreadGrips: function TC_clearPauseGrips() {
    this._clearObjectClients("_threadGrips");
  },

  



  _onThreadState: function TC_onThreadState(aPacket) {
    this._state = ThreadStateTypes[aPacket.type];
    this._clearFrames();
    this._clearPauseGrips();
    aPacket.type === ThreadStateTypes.detached && this._clearThreadGrips();
    this._client._eventsEnabled && this.notify(aPacket.type, aPacket);
  },

  


  environment: function(aForm) {
    return new EnvironmentClient(this._client, aForm);
  },

  


  source: function TC_source(aForm) {
    if (aForm.actor in this._threadGrips) {
      return this._threadGrips[aForm.actor];
    }

    return this._threadGrips[aForm.actor] = new SourceClient(this._client,
                                                             aForm);
  },

  







  getPrototypesAndProperties: DebuggerClient.requester({
    type: "prototypesAndProperties",
    actors: args(0)
  }, {
    telemetry: "PROTOTYPESANDPROPERTIES"
  })
};

eventSource(ThreadClient.prototype);












function TraceClient(aClient, aActor) {
  this._client = aClient;
  this._actor = aActor;
  this._activeTraces = new Set();
  this._waitingPackets = new Map();
  this._expectedPacket = 0;

  this.onPacket = this.onPacket.bind(this);
  this._client.addListener(UnsolicitedNotifications.enteredFrame, this.onPacket);
  this._client.addListener(UnsolicitedNotifications.exitedFrame, this.onPacket);

  this.request = this._client.request;
}

TraceClient.prototype = {
  get actor()   { return this._actor; },
  get tracing() { return this._activeTraces.size > 0; },

  get _transport() { return this._client._transport; },

  


  detach: DebuggerClient.requester({ type: "detach" },
                                   { telemetry: "TRACERDETACH" }),

  











  startTrace: DebuggerClient.requester({
    type: "startTrace",
    name: args(1),
    trace: args(0)
  }, {
    after: function(aResponse) {
      if (aResponse.error) {
        return aResponse;
      }

      if (!this.tracing) {
        this._waitingPackets.clear();
        this._expectedPacket = 0;
      }
      this._activeTraces.add(aResponse.name);

      return aResponse;
    },
    telemetry: "STARTTRACE"
  }),

  









  stopTrace: DebuggerClient.requester({
    type: "stopTrace",
    name: args(0)
  }, {
    after: function(aResponse) {
      if (aResponse.error) {
        return aResponse;
      }

      this._activeTraces.delete(aResponse.name);

      return aResponse;
    },
    telemetry: "STOPTRACE"
  }),

  









  onPacket: function JSTC_onPacket(aEvent, aPacket) {
    this._waitingPackets.set(aPacket.sequence, aPacket);

    while (this._waitingPackets.has(this._expectedPacket)) {
      let packet = this._waitingPackets.get(this._expectedPacket);
      this._waitingPackets.delete(this._expectedPacket);
      this.notify(packet.type, packet);
      this._expectedPacket++;
    }
  }
};

eventSource(TraceClient.prototype);









function ObjectClient(aClient, aGrip)
{
  this._grip = aGrip;
  this._client = aClient;
  this.request = this._client.request;
}

ObjectClient.prototype = {
  get actor() { return this._grip.actor },
  get _transport() { return this._client._transport; },

  valid: true,

  get isFrozen() this._grip.frozen,
  get isSealed() this._grip.sealed,
  get isExtensible() this._grip.extensible,

  







  getParameterNames: DebuggerClient.requester({
    type: "parameterNames"
  }, {
    before: function (aPacket) {
      if (this._grip["class"] !== "Function") {
        throw new Error("getParameterNames is only valid for function grips.");
      }
      return aPacket;
    },
    telemetry: "PARAMETERNAMES"
  }),

  





  getOwnPropertyNames: DebuggerClient.requester({
    type: "ownPropertyNames"
  }, {
    telemetry: "OWNPROPERTYNAMES"
  }),

  




  getPrototypeAndProperties: DebuggerClient.requester({
    type: "prototypeAndProperties"
  }, {
    telemetry: "PROTOTYPEANDPROPERTIES"
  }),

  





  getProperty: DebuggerClient.requester({
    type: "property",
    name: args(0)
  }, {
    telemetry: "PROPERTY"
  }),

  




  getPrototype: DebuggerClient.requester({
    type: "prototype"
  }, {
    telemetry: "PROTOTYPE"
  }),

  




  getDisplayString: DebuggerClient.requester({
    type: "displayString"
  }, {
    telemetry: "DISPLAYSTRING"
  }),

  




  getScope: DebuggerClient.requester({
    type: "scope"
  }, {
    before: function (aPacket) {
      if (this._grip.class !== "Function") {
        throw new Error("scope is only valid for function grips.");
      }
      return aPacket;
    },
    telemetry: "SCOPE"
  })
};










function LongStringClient(aClient, aGrip) {
  this._grip = aGrip;
  this._client = aClient;
  this.request = this._client.request;
}

LongStringClient.prototype = {
  get actor() { return this._grip.actor; },
  get length() { return this._grip.length; },
  get initial() { return this._grip.initial; },
  get _transport() { return this._client._transport; },

  valid: true,

  









  substring: DebuggerClient.requester({
    type: "substring",
    start: args(0),
    end: args(1)
  }, {
    telemetry: "SUBSTRING"
  }),
};









function SourceClient(aClient, aForm) {
  this._form = aForm;
  this._isBlackBoxed = aForm.isBlackBoxed;
  this._client = aClient;
}

SourceClient.prototype = {
  get _transport() this._client._transport,
  get _activeThread() this._client.activeThread,
  get isBlackBoxed() this._isBlackBoxed,
  get actor() this._form.actor,
  get request() this._client.request,
  get url() this._form.url,

  





  blackBox: DebuggerClient.requester({
    type: "blackbox"
  }, {
    telemetry: "BLACKBOX",
    after: function (aResponse) {
      if (!aResponse.error) {
        this._isBlackBoxed = true;
        if (this._activeThread) {
          this._activeThread.notify("blackboxchange", this);
        }
      }
      return aResponse;
    }
  }),

  





  unblackBox: DebuggerClient.requester({
    type: "unblackbox"
  }, {
    telemetry: "UNBLACKBOX",
    after: function (aResponse) {
      if (!aResponse.error) {
        this._isBlackBoxed = false;
        if (this._activeThread) {
          this._activeThread.notify("blackboxchange", this);
        }
      }
      return aResponse;
    }
  }),

  


  source: function SC_source(aCallback) {
    let packet = {
      to: this._form.actor,
      type: "source"
    };
    this._client.request(packet, aResponse => {
      this._onSourceResponse(aResponse, aCallback)
    });
  },

  


  prettyPrint: function SC_prettyPrint(aIndent, aCallback) {
    const packet = {
      to: this._form.actor,
      type: "prettyPrint",
      indent: aIndent
    };
    this._client.request(packet, aResponse => {
      this._onSourceResponse(aResponse, aCallback);
    });
  },

  _onSourceResponse: function SC__onSourceResponse(aResponse, aCallback) {
    if (aResponse.error) {
      aCallback(aResponse);
      return;
    }

    if (typeof aResponse.source === "string") {
      aCallback(aResponse);
      return;
    }

    let { contentType, source } = aResponse;
    let longString = this._client.activeThread.threadLongString(
      source);
    longString.substring(0, longString.length, function (aResponse) {
      if (aResponse.error) {
        aCallback(aResponse);
        return;
      }

      aCallback({
        source: aResponse.substring,
        contentType: contentType
      });
    });
  }
};












function BreakpointClient(aClient, aActor, aLocation) {
  this._client = aClient;
  this._actor = aActor;
  this.location = aLocation;
  this.request = this._client.request;
}

BreakpointClient.prototype = {

  _actor: null,
  get actor() { return this._actor; },
  get _transport() { return this._client._transport; },

  


  remove: DebuggerClient.requester({
    type: "delete"
  }, {
    telemetry: "DELETE"
  }),
};

eventSource(BreakpointClient.prototype);









function EnvironmentClient(aClient, aForm) {
  this._client = aClient;
  this._form = aForm;
  this.request = this._client.request;
}

EnvironmentClient.prototype = {

  get actor() this._form.actor,
  get _transport() { return this._client._transport; },

  


  getBindings: DebuggerClient.requester({
    type: "bindings"
  }, {
    telemetry: "BINDINGS"
  }),

  



  assign: DebuggerClient.requester({
    type: "assign",
    name: args(0),
    value: args(1)
  }, {
    telemetry: "ASSIGN"
  })
};

eventSource(EnvironmentClient.prototype);









this.debuggerSocketConnect = function debuggerSocketConnect(aHost, aPort)
{
  let s = socketTransportService.createTransport(null, 0, aHost, aPort, null);
  let transport = new DebuggerTransport(s.openInputStream(0, 0, 0),
                                        s.openOutputStream(0, 0, 0));
  return transport;
}




function pair(aItemOne, aItemTwo) {
  return [aItemOne, aItemTwo];
}
