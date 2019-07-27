





"use strict";
var Ci = Components.interfaces;
var Cc = Components.classes;
var Cu = Components.utils;
var Cr = Components.results;
var CC = Components.Constructor;


this.Ci = Ci;
this.Cc = Cc;
this.Cu = Cu;
this.Cr = Cr;
this.CC = CC;

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

let promise = Cu.import("resource://gre/modules/devtools/deprecated-sync-thenables.js").Promise;
const { defer, resolve, reject } = promise;

XPCOMUtils.defineLazyServiceGetter(this, "socketTransportService",
                                   "@mozilla.org/network/socket-transport-service;1",
                                   "nsISocketTransportService");

XPCOMUtils.defineLazyModuleGetter(this, "console",
                                  "resource://gre/modules/devtools/Console.jsm");

XPCOMUtils.defineLazyModuleGetter(this, "devtools",
                                  "resource://gre/modules/devtools/Loader.jsm");

XPCOMUtils.defineLazyGetter(this, "events", () => {
  return devtools.require("sdk/event/core");
});

Object.defineProperty(this, "WebConsoleClient", {
  get: function () {
    return devtools.require("devtools/toolkit/webconsole/client").WebConsoleClient;
  },
  configurable: true,
  enumerable: true
});

Components.utils.import("resource://gre/modules/devtools/DevToolsUtils.jsm");
this.makeInfallible = DevToolsUtils.makeInfallible;

let LOG_PREF = "devtools.debugger.log";
let VERBOSE_PREF = "devtools.debugger.log.verbose";
let wantLogging = Services.prefs.getBoolPref(LOG_PREF);
let wantVerbose =
  Services.prefs.getPrefType(VERBOSE_PREF) !== Services.prefs.PREF_INVALID &&
  Services.prefs.getBoolPref(VERBOSE_PREF);

const noop = () => {};

function dumpn(str) {
  if (wantLogging) {
    dump("DBG-CLIENT: " + str + "\n");
  }
}

function dumpv(msg) {
  if (wantVerbose) {
    dumpn(msg);
  }
}

let loader = Cc["@mozilla.org/moz/jssubscript-loader;1"]
  .getService(Ci.mozIJSSubScriptLoader);
loader.loadSubScript("resource://gre/modules/devtools/transport/transport.js", this);












function eventSource(aProto) {
  









  aProto.addListener = function (aName, aListener) {
    if (typeof aListener != "function") {
      throw TypeError("Listeners must be functions.");
    }

    if (!this._listeners) {
      this._listeners = {};
    }

    this._getListeners(aName).push(aListener);
  };

  








  aProto.addOneTimeListener = function (aName, aListener) {
    let l = (...args) => {
      this.removeListener(aName, l);
      aListener.apply(null, args);
    };
    this.addListener(aName, l);
  };

  









  aProto.removeListener = function (aName, aListener) {
    if (!this._listeners || !this._listeners[aName]) {
      return;
    }
    this._listeners[aName] =
      this._listeners[aName].filter(function (l) { return l != aListener });
  };

  






  aProto._getListeners = function (aName) {
    if (aName in this._listeners) {
      return this._listeners[aName];
    }
    this._listeners[aName] = [];
    return this._listeners[aName];
  };

  








  aProto.emit = function () {
    if (!this._listeners) {
      return;
    }

    let name = arguments[0];
    let listeners = this._getListeners(name).slice(0);

    for each (let listener in listeners) {
      try {
        listener.apply(null, arguments);
      } catch (e) {
        
        DevToolsUtils.reportException("notify event '" + name + "'", e);
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
  "reflowActivity": "reflowActivity",
  "addonListChanged": "addonListChanged",
  "tabNavigated": "tabNavigated",
  "frameUpdate": "frameUpdate",
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






this.DebuggerClient = function (aTransport)
{
  this._transport = aTransport;
  this._transport.hooks = this;

  
  this._clients = new Map;

  this._pendingRequests = [];
  this._activeRequests = new Map;
  this._eventsEnabled = true;

  this.compat = new ProtocolCompatibility(this, []);
  this.traits = {};

  this.request = this.request.bind(this);
  this.localTransport = this._transport.onOutputStreamReady === undefined;

  



  this.mainRoot = null;
  this.expectReply("root", (aPacket) => {
    this.mainRoot = new RootClient(this, aPacket);
    this.emit("connected", aPacket.applicationType, aPacket.traits);
  });
}



















DebuggerClient.requester = function (aPacketSkeleton,
                                     { telemetry, before, after }) {
  return DevToolsUtils.makeInfallible(function (...args) {
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

    this.request(outgoingPacket, DevToolsUtils.makeInfallible((aResponse) => {
      if (after) {
        let { from } = aResponse;
        aResponse = after.call(this, aResponse);
        if (!aResponse.from) {
          aResponse.from = from;
        }
      }

      
      let thisCallback = args[maxPosition + 1];
      if (thisCallback) {
        thisCallback(aResponse);
      }

      if (histogram) {
        histogram.add(+new Date - startTime);
      }
    }, "DebuggerClient.requester request callback"));

  }, "DebuggerClient.requester");
};

function args(aPos) {
  return new DebuggerClient.Argument(aPos);
}

DebuggerClient.Argument = function (aPosition) {
  this.position = aPosition;
};

DebuggerClient.Argument.prototype.getArgument = function (aParams) {
  if (!(this.position in aParams)) {
    throw new Error("Bad index into params: " + this.position);
  }
  return aParams[this.position];
};

DebuggerClient.prototype = {
  






  connect: function (aOnConnected) {
    this.addOneTimeListener("connected", (aName, aApplicationType, aTraits) => {
      this.traits = aTraits;
      if (aOnConnected) {
        aOnConnected(aApplicationType, aTraits);
      }
    });

    this._transport.ready();
  },

  






  close: function (aOnClosed) {
    
    
    this._eventsEnabled = false;

    if (aOnClosed) {
      this.addOneTimeListener('closed', function (aEvent) {
        aOnClosed();
      });
    }

    
    
    
    let clients = [...this._clients.values()];
    this._clients.clear();
    const detachClients = () => {
      let client = clients.pop();
      if (!client) {
        
        this._transport.close();
        this._transport = null;
        return;
      }
      if (client.detach) {
        client.detach(detachClients);
        return;
      }
      detachClients();
    };
    detachClients();
  },

  



  listTabs: function (aOnResponse) { return this.mainRoot.listTabs(aOnResponse); },

  



  listAddons: function (aOnResponse) { return this.mainRoot.listAddons(aOnResponse); },

  








  attachTab: function (aTabActor, aOnResponse = noop) {
    if (this._clients.has(aTabActor)) {
      let cachedTab = this._clients.get(aTabActor);
      let cachedResponse = {
        cacheDisabled: cachedTab.cacheDisabled,
        javascriptEnabled: cachedTab.javascriptEnabled,
        traits: cachedTab.traits,
      };
      DevToolsUtils.executeSoon(() => aOnResponse(cachedResponse, cachedTab));
      return;
    }

    let packet = {
      to: aTabActor,
      type: "attach"
    };
    this.request(packet, (aResponse) => {
      let tabClient;
      if (!aResponse.error) {
        tabClient = new TabClient(this, aResponse);
        this.registerClient(tabClient);
      }
      aOnResponse(aResponse, tabClient);
    });
  },

  








  attachAddon: function DC_attachAddon(aAddonActor, aOnResponse = noop) {
    let packet = {
      to: aAddonActor,
      type: "attach"
    };
    this.request(packet, aResponse => {
      let addonClient;
      if (!aResponse.error) {
        addonClient = new AddonClient(this, aAddonActor);
        this.registerClient(addonClient);
        this.activeAddon = addonClient;
      }
      aOnResponse(aResponse, addonClient);
    });
  },

  










  attachConsole:
  function (aConsoleActor, aListeners, aOnResponse = noop) {
    let packet = {
      to: aConsoleActor,
      type: "startListeners",
      listeners: aListeners,
    };

    this.request(packet, (aResponse) => {
      let consoleClient;
      if (!aResponse.error) {
        if (this._clients.has(aConsoleActor)) {
          consoleClient = this._clients.get(aConsoleActor);
        } else {
          consoleClient = new WebConsoleClient(this, aResponse);
          this.registerClient(consoleClient);
        }
      }
      aOnResponse(aResponse, consoleClient);
    });
  },

  











  attachThread: function (aThreadActor, aOnResponse = noop, aOptions={}) {
    if (this._clients.has(aThreadActor)) {
      DevToolsUtils.executeSoon(() => aOnResponse({}, this._clients.get(aThreadActor)));
      return;
    }

   let packet = {
      to: aThreadActor,
      type: "attach",
      options: aOptions
    };
    this.request(packet, (aResponse) => {
      if (!aResponse.error) {
        var threadClient = new ThreadClient(this, aThreadActor);
        this.registerClient(threadClient);
      }
      aOnResponse(aResponse, threadClient);
    });
  },

  








  attachTracer: function (aTraceActor, aOnResponse = noop) {
    if (this._clients.has(aTraceActor)) {
      DevToolsUtils.executeSoon(() => aOnResponse({}, this._clients.get(aTraceActor)));
      return;
    }

    let packet = {
      to: aTraceActor,
      type: "attach"
    };
    this.request(packet, (aResponse) => {
      if (!aResponse.error) {
        var traceClient = new TraceClient(this, aTraceActor);
        this.registerClient(traceClient);
      }
      aOnResponse(aResponse, traceClient);
    });
  },

  








  release: DebuggerClient.requester({
    to: args(0),
    type: "release"
  }, {
    telemetry: "RELEASE"
  }),

  














































  request: function (aRequest, aOnResponse) {
    if (!this.mainRoot) {
      throw Error("Have not yet received a hello packet from the server.");
    }
    if (!aRequest.to) {
      let type = aRequest.type || "";
      throw Error("'" + type + "' request packet has no destination.");
    }

    let request = new Request(aRequest);
    request.format = "json";
    if (aOnResponse) {
      request.on("json-reply", aOnResponse);
    }

    this._pendingRequests.push(request);
    this._sendRequests();

    
    
    let deferred = promise.defer();
    function listenerJson(resp) {
      request.off("json-reply", listenerJson);
      request.off("bulk-reply", listenerBulk);
      if (resp.error) {
        deferred.reject(resp);
      } else {
        deferred.resolve(resp);
      }
    }
    function listenerBulk(resp) {
      request.off("json-reply", listenerJson);
      request.off("bulk-reply", listenerBulk);
      deferred.resolve(resp);
    }
    request.on("json-reply", listenerJson);
    request.on("bulk-reply", listenerBulk);
    request.then = deferred.promise.then.bind(deferred.promise);

    return request;
  },

  








































































  startBulkRequest: function(request) {
    if (!this.traits.bulk) {
      throw Error("Server doesn't support bulk transfers");
    }
    if (!this.mainRoot) {
      throw Error("Have not yet received a hello packet from the server.");
    }
    if (!request.type) {
      throw Error("Bulk packet is missing the required 'type' field.");
    }
    if (!request.actor) {
      throw Error("'" + request.type + "' bulk packet has no destination.");
    }
    if (!request.length) {
      throw Error("'" + request.type + "' bulk packet has no length.");
    }

    request = new Request(request);
    request.format = "bulk";

    this._pendingRequests.push(request);
    this._sendRequests();

    return request;
  },

  



  _sendRequests: function () {
    this._pendingRequests = this._pendingRequests.filter((request) => {
      let dest = request.actor;

      if (this._activeRequests.has(dest)) {
        return true;
      }

      this.expectReply(dest, request);

      if (request.format === "json") {
        this._transport.send(request.request);
        return false;
      }

      this._transport.startBulkSend(request.request).then((...args) => {
        request.emit("bulk-send-ready", ...args);
      });

      return false;
    });
  },

  








  expectReply: function (aActor, aRequest) {
    if (this._activeRequests.has(aActor)) {
      throw Error("clashing handlers for next reply from " + uneval(aActor));
    }

    
    
    if (typeof aRequest === "function") {
      let handler = aRequest;
      aRequest = new Request();
      aRequest.on("json-reply", handler);
    }

    this._activeRequests.set(aActor, aRequest);
  },

  

  







  onPacket: function (aPacket, aIgnoreCompatibility=false) {
    let packet = aIgnoreCompatibility
      ? aPacket
      : this.compat.onPacket(aPacket);

    resolve(packet).then(aPacket => {
      if (!aPacket.from) {
        DevToolsUtils.reportException(
          "onPacket",
          new Error("Server did not specify an actor, dropping packet: " +
                    JSON.stringify(aPacket)));
        return;
      }

      
      
      let front = this.getActor(aPacket.from);
      if (front) {
        front.onPacket(aPacket);
        return;
      }

      if (this._clients.has(aPacket.from) && aPacket.type) {
        let client = this._clients.get(aPacket.from);
        let type = aPacket.type;
        if (client.events.indexOf(type) != -1) {
          client.emit(type, aPacket);
          
          return;
        }
      }

      let activeRequest;
      
      
      
      if (this._activeRequests.has(aPacket.from) &&
          !(aPacket.type in UnsolicitedNotifications) &&
          !(aPacket.type == ThreadStateTypes.paused &&
            aPacket.why.type in UnsolicitedPauses)) {
        activeRequest = this._activeRequests.get(aPacket.from);
        this._activeRequests.delete(aPacket.from);
      }

      
      if (aPacket.type in ThreadStateTypes &&
          this._clients.has(aPacket.from) &&
          typeof this._clients.get(aPacket.from)._onThreadState == "function") {
        this._clients.get(aPacket.from)._onThreadState(aPacket);
      }
      
      
      
      if (aPacket.type == UnsolicitedNotifications.tabNavigated &&
          this._clients.has(aPacket.from) &&
          this._clients.get(aPacket.from).thread) {
        let thread = this._clients.get(aPacket.from).thread;
        let resumption = { from: thread._actor, type: "resumed" };
        thread._onThreadState(resumption);
      }
      
      
      if (aPacket.type) {
        this.emit(aPacket.type, aPacket);
      }

      if (activeRequest) {
        activeRequest.emit("json-reply", aPacket);
      }

      this._sendRequests();
    }, ex => DevToolsUtils.reportException("onPacket handler", ex));
  },

  





























  onBulkPacket: function(packet) {
    let { actor, type, length } = packet;

    if (!actor) {
      DevToolsUtils.reportException(
        "onBulkPacket",
        new Error("Server did not specify an actor, dropping bulk packet: " +
                  JSON.stringify(packet)));
      return;
    }

    
    
    if (!this._activeRequests.has(actor)) {
      return;
    }

    let activeRequest = this._activeRequests.get(actor);
    this._activeRequests.delete(actor);
    activeRequest.emit("bulk-reply", packet);

    this._sendRequests();
  },

  






  onClosed: function (aStatus) {
    this.emit("closed");
  },

  registerClient: function (client) {
    let actorID = client.actor;
    if (!actorID) {
      throw new Error("DebuggerServer.registerClient expects " +
                      "a client instance with an `actor` attribute.");
    }
    if (!Array.isArray(client.events)) {
      throw new Error("DebuggerServer.registerClient expects " +
                      "a client instance with an `events` attribute " +
                      "that is an array.");
    }
    if (client.events.length > 0 && typeof(client.emit) != "function") {
      throw new Error("DebuggerServer.registerClient expects " +
                      "a client instance with non-empty `events` array to" +
                      "have an `emit` function.");
    }
    if (this._clients.has(actorID)) {
      throw new Error("DebuggerServer.registerClient already registered " +
                      "a client for this actor.");
    }
    this._clients.set(actorID, client);
  },

  unregisterClient: function (client) {
    let actorID = client.actor;
    if (!actorID) {
      throw new Error("DebuggerServer.unregisterClient expects " +
                      "a Client instance with a `actor` attribute.");
    }
    this._clients.delete(actorID);
  },

  


  __pools: null,
  get _pools() {
    if (this.__pools) {
      return this.__pools;
    }
    this.__pools = new Set();
    return this.__pools;
  },

  addActorPool: function (pool) {
    this._pools.add(pool);
  },
  removeActorPool: function (pool) {
    this._pools.delete(pool);
  },
  getActor: function (actorID) {
    let pool = this.poolFor(actorID);
    return pool ? pool.get(actorID) : null;
  },

  poolFor: function (actorID) {
    for (let pool of this._pools) {
      if (pool.has(actorID)) return pool;
    }
    return null;
  },

  


  activeAddon: null
}

eventSource(DebuggerClient.prototype);

function Request(request) {
  this.request = request;
}

Request.prototype = {

  on: function(type, listener) {
    events.on(this, type, listener);
  },

  off: function(type, listener) {
    events.off(this, type, listener);
  },

  once: function(type, listener) {
    events.once(this, type, listener);
  },

  emit: function(type, ...args) {
    events.emit(this, type, ...args);
  },

  get actor() { return this.request.to || this.request.actor; }

};


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
  






  supportsFeature: function (aFeatureName) {
    return this._featureDeferreds[aFeatureName].promise;
  },

  





  rejectFeature: function (aFeatureName) {
    this._featureDeferreds[aFeatureName].reject(false);
  },

  






  onPacket: function (aPacket) {
    this._detectFeatures(aPacket);
    return this._shimPacket(aPacket);
  },

  



  _detectFeatures: function (aPacket) {
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
          DevToolsUtils.reportException(
            "PC__detectFeatures",
            new Error("Bad return value from `onPacketTest` for feature '"
                      + feature.name + "'"));
        }
      } catch (ex) {
        DevToolsUtils.reportException("PC__detectFeatures", ex);
      }
    }
  },

  



  _shimPacket: function (aPacket) {
    let extraPackets = [];

    let loop = (aFeatures, aPacket) => {
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
    };

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











function TabClient(aClient, aForm) {
  this.client = aClient;
  this._actor = aForm.from;
  this._threadActor = aForm.threadActor;
  this.javascriptEnabled = aForm.javascriptEnabled;
  this.cacheDisabled = aForm.cacheDisabled;
  this.thread = null;
  this.request = this.client.request;
  this.traits = aForm.traits || {};
  this.events = [];
}

TabClient.prototype = {
  get actor() { return this._actor },
  get _transport() { return this.client._transport; },

  









  attachThread: function(aOptions={}, aOnResponse = noop) {
    if (this.thread) {
      DevToolsUtils.executeSoon(() => aOnResponse({}, this.thread));
      return;
    }

    let packet = {
      to: this._threadActor,
      type: "attach",
      options: aOptions
    };
    this.request(packet, (aResponse) => {
      if (!aResponse.error) {
        this.thread = new ThreadClient(this, this._threadActor);
        this.client.registerClient(this.thread);
      }
      aOnResponse(aResponse, this.thread);
    });
  },

  





  detach: DebuggerClient.requester({
    type: "detach"
  }, {
    before: function (aPacket) {
      if (this.thread) {
        this.thread.detach();
      }
      return aPacket;
    },
    after: function (aResponse) {
      this.client.unregisterClient(this);
      return aResponse;
    },
    telemetry: "TABDETACH"
  }),

  






  reload: function(options = { force: false }) {
    return this._reload(options);
  },
  _reload: DebuggerClient.requester({
    type: "reload",
    options: args(0)
  }, {
    telemetry: "RELOAD"
  }),

  





  navigateTo: DebuggerClient.requester({
    type: "navigateTo",
    url: args(0)
  }, {
    telemetry: "NAVIGATETO"
  }),

  







  reconfigure: DebuggerClient.requester({
    type: "reconfigure",
    options: args(0)
  }, {
    telemetry: "RECONFIGURETAB"
  }),
};

eventSource(TabClient.prototype);

function AddonClient(aClient, aActor) {
  this._client = aClient;
  this._actor = aActor;
  this.request = this._client.request;
  this.events = [];
}

AddonClient.prototype = {
  get actor() { return this._actor; },
  get _transport() { return this._client._transport; },

  





  detach: DebuggerClient.requester({
    type: "detach"
  }, {
    after: function(aResponse) {
      if (this._client.activeAddon === this) {
        this._client.activeAddon = null
      }
      this._client.unregisterClient(this);
      return aResponse;
    },
    telemetry: "ADDONDETACH"
  })
};





















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
  this._parent = aClient;
  this.client = aClient instanceof DebuggerClient ? aClient : aClient.client;
  this._actor = aActor;
  this._frameCache = [];
  this._scriptCache = {};
  this._pauseGrips = {};
  this._threadGrips = {};
  this.request = this.client.request;
  this.events = [];
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

  get compat() { return this.client.compat; },
  get _transport() { return this.client._transport; },

  _assertPaused: function (aCommand) {
    if (!this.paused) {
      throw Error(aCommand + " command sent while not paused. Currently " + this._state);
    }
  },

  










  _doResume: DebuggerClient.requester({
    type: "resume",
    resumeLimit: args(0)
  }, {
    before: function (aPacket) {
      this._assertPaused("resume");

      
      
      this._state = "resuming";

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

  







  reconfigure: DebuggerClient.requester({
    type: "reconfigure",
    options: args(0)
  }, {
    telemetry: "RECONFIGURETHREAD"
  }),

  


  resume: function (aOnResponse) {
    this._doResume(null, aOnResponse);
  },

  





  breakOnNext: function (aOnResponse) {
    this._doResume({ type: "break" }, aOnResponse);
  },

  





  stepOver: function (aOnResponse) {
    this._doResume({ type: "next" }, aOnResponse);
  },

  





  stepIn: function (aOnResponse) {
    this._doResume({ type: "step" }, aOnResponse);
  },

  





  stepOut: function (aOnResponse) {
    this._doResume({ type: "finish" }, aOnResponse);
  },

  





  interrupt: DebuggerClient.requester({
    type: "interrupt"
  }, {
    telemetry: "INTERRUPT"
  }),

  







  pauseOnExceptions: function (aPauseOnExceptions,
                               aIgnoreCaughtExceptions,
                               aOnResponse = noop) {
    this._pauseOnExceptions = aPauseOnExceptions;
    this._ignoreCaughtExceptions = aIgnoreCaughtExceptions;

    
    
    if (this.paused) {
      this.reconfigure({
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

  











  pauseOnDOMEvents: function (events, onResponse = noop) {
    this._pauseOnDOMEvents = events;
    
    
    
    if (this.paused) {
      DevToolsUtils.executeSoon(() => onResponse({}));
      return;
    }
    this.interrupt(response => {
      
      if (response.error) {
        onResponse(response);
        return;
      }
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
        
        this._state = "paused";
      }
      return aResponse;
    },
    telemetry: "CLIENTEVALUATE"
  }),

  





  detach: DebuggerClient.requester({
    type: "detach"
  }, {
    after: function (aResponse) {
      this.client.unregisterClient(this);
      this._parent.thread = null;
      return aResponse;
    },
    telemetry: "THREADDETACH"
  }),

  







  setBreakpoint: function ({ url, line, column, condition },
                           aOnResponse = noop) {
    
    let doSetBreakpoint = (aCallback) => {
      const location = {
        url: url,
        line: line,
        column: column
      };

      let packet = {
        to: this._actor,
        type: "setBreakpoint",
        location: location,
        condition: condition
      };
      this.client.request(packet, (aResponse) => {
        
        
        let bpClient;
        if (aResponse.actor) {
          let root = this.client.mainRoot;
          bpClient = new BreakpointClient(
            this.client,
            aResponse.actor,
            location,
            root.traits.conditionalBreakpoints ? condition : undefined
          );
        }
        aOnResponse(aResponse, bpClient);
        if (aCallback) {
          aCallback();
        }
      });
    };

    
    if (this.paused) {
      doSetBreakpoint();
      return;
    }
    
    this.interrupt((aResponse) => {
      if (aResponse.error) {
        
        aOnResponse(aResponse);
        return;
      }

      const { type, why } = aResponse;
      const cleanUp = type == "paused" && why.type == "interrupted"
        ? () => this.resume()
        : noop;

      doSetBreakpoint(cleanUp);
    });
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

  





  getSources: DebuggerClient.requester({
    type: "sources"
  }, {
    telemetry: "SOURCES"
  }),

  



  _clearScripts: function () {
    if (Object.keys(this._scriptCache).length > 0) {
      this._scriptCache = {}
      this.emit("scriptscleared");
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

  










  fillFrames: function (aTotal, aCallback=noop) {
    this._assertPaused("fillFrames");
    if (this._frameCache.length >= aTotal) {
      return false;
    }

    let numFrames = this._frameCache.length;

    this.getFrames(numFrames, aTotal - numFrames, (aResponse) => {
      if (aResponse.error) {
        aCallback(aResponse);
        return;
      }

      for each (let frame in aResponse.frames) {
        this._frameCache[frame.depth] = frame;
      }

      
      
      this.emit("framesadded");

      aCallback(aResponse);
    });

    return true;
  },

  



  _clearFrames: function () {
    if (this._frameCache.length > 0) {
      this._frameCache = [];
      this.emit("framescleared");
    }
  },

  





  pauseGrip: function (aGrip) {
    if (aGrip.actor in this._pauseGrips) {
      return this._pauseGrips[aGrip.actor];
    }

    let client = new ObjectClient(this.client, aGrip);
    this._pauseGrips[aGrip.actor] = client;
    return client;
  },

  









  _longString: function (aGrip, aGripCacheName) {
    if (aGrip.actor in this[aGripCacheName]) {
      return this[aGripCacheName][aGrip.actor];
    }

    let client = new LongStringClient(this.client, aGrip);
    this[aGripCacheName][aGrip.actor] = client;
    return client;
  },

  






  pauseLongString: function (aGrip) {
    return this._longString(aGrip, "_pauseGrips");
  },

  






  threadLongString: function (aGrip) {
    return this._longString(aGrip, "_threadGrips");
  },

  





  _clearObjectClients: function (aGripCacheName) {
    for each (let grip in this[aGripCacheName]) {
      grip.valid = false;
    }
    this[aGripCacheName] = {};
  },

  



  _clearPauseGrips: function () {
    this._clearObjectClients("_pauseGrips");
  },

  



  _clearThreadGrips: function () {
    this._clearObjectClients("_threadGrips");
  },

  



  _onThreadState: function (aPacket) {
    this._state = ThreadStateTypes[aPacket.type];
    this._clearFrames();
    this._clearPauseGrips();
    aPacket.type === ThreadStateTypes.detached && this._clearThreadGrips();
    this.client._eventsEnabled && this.emit(aPacket.type, aPacket);
  },

  


  environment: function (aForm) {
    return new EnvironmentClient(this.client, aForm);
  },

  


  source: function (aForm) {
    if (aForm.actor in this._threadGrips) {
      return this._threadGrips[aForm.actor];
    }

    return this._threadGrips[aForm.actor] = new SourceClient(this, aForm);
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
  this.request = this._client.request;
  this.events = [];
}

TraceClient.prototype = {
  get actor()   { return this._actor; },
  get tracing() { return this._activeTraces.size > 0; },

  get _transport() { return this._client._transport; },

  


  detach: DebuggerClient.requester({
    type: "detach"
  }, {
    after: function (aResponse) {
      this._client.unregisterClient(this);
      return aResponse;
    },
    telemetry: "TRACERDETACH"
  }),

  











  startTrace: DebuggerClient.requester({
    type: "startTrace",
    name: args(1),
    trace: args(0)
  }, {
    after: function (aResponse) {
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
    after: function (aResponse) {
      if (aResponse.error) {
        return aResponse;
      }

      this._activeTraces.delete(aResponse.name);

      return aResponse;
    },
    telemetry: "STOPTRACE"
  })
};









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

  getDefinitionSite: DebuggerClient.requester({
    type: "definitionSite"
  }, {
    before: function (aPacket) {
      if (this._grip.class != "Function") {
        throw new Error("getDefinitionSite is only valid for function grips.");
      }
      return aPacket;
    }
  }),

  







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
  this._isPrettyPrinted = aForm.isPrettyPrinted;
  this._activeThread = aClient;
  this._client = aClient.client;
}

SourceClient.prototype = {
  get _transport() this._client._transport,
  get isBlackBoxed() this._isBlackBoxed,
  get isPrettyPrinted() this._isPrettyPrinted,
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
          this._activeThread.emit("blackboxchange", this);
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
          this._activeThread.emit("blackboxchange", this);
        }
      }
      return aResponse;
    }
  }),

  


  source: function (aCallback) {
    let packet = {
      to: this._form.actor,
      type: "source"
    };
    this._client.request(packet, aResponse => {
      this._onSourceResponse(aResponse, aCallback)
    });
  },

  


  prettyPrint: function (aIndent, aCallback) {
    const packet = {
      to: this._form.actor,
      type: "prettyPrint",
      indent: aIndent
    };
    this._client.request(packet, aResponse => {
      if (!aResponse.error) {
        this._isPrettyPrinted = true;
        this._activeThread._clearFrames();
        this._activeThread.emit("prettyprintchange", this);
      }
      this._onSourceResponse(aResponse, aCallback);
    });
  },

  


  disablePrettyPrint: function (aCallback) {
    const packet = {
      to: this._form.actor,
      type: "disablePrettyPrint"
    };
    this._client.request(packet, aResponse => {
      if (!aResponse.error) {
        this._isPrettyPrinted = false;
        this._activeThread._clearFrames();
        this._activeThread.emit("prettyprintchange", this);
      }
      this._onSourceResponse(aResponse, aCallback);
    });
  },

  _onSourceResponse: function (aResponse, aCallback) {
    if (aResponse.error) {
      aCallback(aResponse);
      return;
    }

    if (typeof aResponse.source === "string") {
      aCallback(aResponse);
      return;
    }

    let { contentType, source } = aResponse;
    let longString = this._activeThread.threadLongString(source);
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














function BreakpointClient(aClient, aActor, aLocation, aCondition) {
  this._client = aClient;
  this._actor = aActor;
  this.location = aLocation;
  this.request = this._client.request;

  
  if (aCondition) {
    this.condition = aCondition;
  }
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

  


  hasCondition: function() {
    let root = this._client.mainRoot;
    
    
    if (root.traits.conditionalBreakpoints) {
      return "condition" in this;
    } else {
      return "conditionalExpression" in this;
    }
  },

  






  getCondition: function() {
    let root = this._client.mainRoot;
    if (root.traits.conditionalBreakpoints) {
      return this.condition;
    } else {
      return this.conditionalExpression;
    }
  },

  


  setCondition: function(gThreadClient, aCondition) {
    let root = this._client.mainRoot;
    let deferred = promise.defer();

    if (root.traits.conditionalBreakpoints) {
      let info = {
        url: this.location.url,
        line: this.location.line,
        column: this.location.column,
        condition: aCondition
      };

      
      
      this.remove(aResponse => {
        if (aResponse && aResponse.error) {
          deferred.reject(aResponse);
          return;
        }

        gThreadClient.setBreakpoint(info, (aResponse, aNewBreakpoint) => {
          if (aResponse && aResponse.error) {
            deferred.reject(aResponse);
          } else {
            deferred.resolve(aNewBreakpoint);
          }
        });
      });
    } else {
      
      if(aCondition === "") {
        delete this.conditionalExpression;
      }
      else {
        this.conditionalExpression = aCondition;
      }
      deferred.resolve(this);
    }

    return deferred.promise;
  }
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









this.debuggerSocketConnect = function (aHost, aPort)
{
  let s = socketTransportService.createTransport(null, 0, aHost, aPort, null);
  
  
  
  s.setTimeout(Ci.nsISocketTransport.TIMEOUT_CONNECT, 2);

  
  
  
  let transport;
  try {
    transport = new DebuggerTransport(s.openInputStream(0, 0, 0),
                                      s.openOutputStream(0, 0, 0));
  } catch(e) {
    DevToolsUtils.reportException("debuggerSocketConnect", e);
    throw e;
  }
  return transport;
}
