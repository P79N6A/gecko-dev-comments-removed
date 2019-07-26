





"use strict";
const Ci = Components.interfaces;
const Cc = Components.classes;
const Cu = Components.utils;
const Cr = Components.results;

this.EXPORTED_SYMBOLS = ["DebuggerTransport",
                         "DebuggerClient",
                         "debuggerSocketConnect",
                         "LongStringClient",
                         "GripClient"];

Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://gre/modules/NetUtil.jsm");
Cu.import("resource://gre/modules/Services.jsm");
Cu.import("resource://gre/modules/commonjs/sdk/core/promise.js");
const { defer, resolve, reject } = Promise;

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
  "networkEvent": "networkEvent",
  "networkEventUpdate": "networkEventUpdate",
  "newGlobal": "newGlobal",
  "newScript": "newScript",
  "newSource": "newSource",
  "tabDetached": "tabDetached",
  "tabNavigated": "tabNavigated",
  "pageError": "pageError",
  "webappsEvent": "webappsEvent",
  "styleSheetsAdded": "styleSheetsAdded"
};





const UnsolicitedPauses = {
  "resumeLimit": "resumeLimit",
  "debuggerStatement": "debuggerStatement",
  "breakpoint": "breakpoint",
  "watchpoint": "watchpoint",
  "exception": "exception"
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

  this.compat = new ProtocolCompatibility(this, [
    new SourcesShim(),
  ]);

  this.request = this.request.bind(this);
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
        thisCallback(aResponse);
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
  if (!this.position in aParams) {
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

  





  listTabs: DebuggerClient.requester({
    to: ROOT_ACTOR_NAME,
    type: "listTabs"
  }, {
    telemetry: "LISTTABS"
  }),

  








  attachTab: function DC_attachTab(aTabActor, aOnResponse) {
    let self = this;
    let packet = {
      to: aTabActor,
      type: "attach"
    };
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

  











  attachThread: function DC_attachThread(aThreadActor, aOnResponse, aOptions={}) {
    let self = this;
    let packet = {
      to: aThreadActor,
      type: "attach",
      options: aOptions
    };
    this.request(packet, function(aResponse) {
      if (!aResponse.error) {
        var threadClient = new ThreadClient(self, aThreadActor);
        self._threadClients[aThreadActor] = threadClient;
        self.activeThread = threadClient;
      }
      aOnResponse(aResponse, threadClient);
    });
  },

  







  reconfigureThread: function DC_reconfigureThread(aUseSourceMaps, aOnResponse) {
    let packet = {
      to: this.activeThread._actor,
      type: "reconfigure",
      options: { useSourceMaps: aUseSourceMaps }
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
    this._pendingRequests = this._pendingRequests.filter(function (request) {
      if (request.to in self._activeRequests) {
        return true;
      }

      self._activeRequests[request.to] = request;
      self._transport.send(request.request);

      return false;
    });
  },

  

  







  onPacket: function DC_onPacket(aPacket, aIgnoreCompatibility=false) {
    let packet = aIgnoreCompatibility
      ? aPacket
      : this.compat.onPacket(aPacket);

    resolve(packet).then((aPacket) => {
      if (!this._connected) {
        
        this._connected = true;
        this.notify("connected",
                    aPacket.applicationType,
                    aPacket.traits);
        return;
      }

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
};

eventSource(TabClient.prototype);











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

      aPacket.pauseOnExceptions = this._pauseOnExceptions;
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

  


  source: function TC_source(aForm) {
    return new SourceClient(this._client, aForm);
  }

};

eventSource(ThreadClient.prototype);









function GripClient(aClient, aGrip)
{
  this._grip = aGrip;
  this._client = aClient;
  this.request = this._client.request;
}

GripClient.prototype = {
  get actor() { return this._grip.actor },
  get _transport() { return this._client._transport; },

  valid: true,

  







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
  this._client = aClient;
}

SourceClient.prototype = {
  get _transport() { return this._client._transport; },

  


  source: function SC_source(aCallback) {
    let packet = {
      to: this._form.actor,
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
