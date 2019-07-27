





"use strict";





let { Ci, Cc, CC, Cu, Cr } = require("chrome");
let Services = require("Services");
let { ActorPool, OriginalLocation, RegisteredActorFactory,
      ObservedActorFactory } = require("devtools/server/actors/common");
let { LocalDebuggerTransport, ChildDebuggerTransport } =
  require("devtools/toolkit/transport/transport");
let DevToolsUtils = require("devtools/toolkit/DevToolsUtils");
let { dumpn, dumpv, dbg_assert } = DevToolsUtils;
let EventEmitter = require("devtools/toolkit/event-emitter");
let Debugger = require("Debugger");

DevToolsUtils.defineLazyGetter(this, "DebuggerSocket", () => {
  let { DebuggerSocket } = require("devtools/toolkit/security/socket");
  return DebuggerSocket;
});
DevToolsUtils.defineLazyGetter(this, "Authentication", () => {
  return require("devtools/toolkit/security/auth");
});




this.Ci = Ci;
this.Cc = Cc;
this.CC = CC;
this.Cu = Cu;
this.Cr = Cr;
this.Services = Services;
this.ActorPool = ActorPool;
this.DevToolsUtils = DevToolsUtils;
this.dumpn = dumpn;
this.dumpv = dumpv;
this.dbg_assert = dbg_assert;



Object.defineProperty(this, "Components", {
  get: function () require("chrome").components
});

if (isWorker) {
  dumpn.wantLogging = true;
  dumpv.wantVerbose = true;
} else {
  const LOG_PREF = "devtools.debugger.log";
  const VERBOSE_PREF = "devtools.debugger.log.verbose";

  dumpn.wantLogging = Services.prefs.getBoolPref(LOG_PREF);
  dumpv.wantVerbose =
    Services.prefs.getPrefType(VERBOSE_PREF) !== Services.prefs.PREF_INVALID &&
    Services.prefs.getBoolPref(VERBOSE_PREF);
}

function loadSubScript(aURL)
{
  try {
    let loader = Cc["@mozilla.org/moz/jssubscript-loader;1"]
      .getService(Ci.mozIJSSubScriptLoader);
    loader.loadSubScript(aURL, this);
  } catch(e) {
    let errorStr = "Error loading: " + aURL + ":\n" +
                   (e.fileName ? "at " + e.fileName + " : " + e.lineNumber + "\n" : "") +
                   e + " - " + e.stack + "\n";
    dump(errorStr);
    reportError(errorStr);
    throw e;
  }
}

loader.lazyRequireGetter(this, "events", "sdk/event/core");

let {defer, resolve, reject, all} = require("devtools/toolkit/deprecated-sync-thenables");
this.defer = defer;
this.resolve = resolve;
this.reject = reject;
this.all = all;

var gRegisteredModules = Object.create(null);









function ModuleAPI() {
  let activeTabActors = new Set();
  let activeGlobalActors = new Set();

  return {
    
    setRootActor: function(factory) {
      DebuggerServer.setRootActor(factory);
    },

    
    addGlobalActor: function(factory, name) {
      DebuggerServer.addGlobalActor(factory, name);
      activeGlobalActors.add(factory);
    },
    
    removeGlobalActor: function(factory) {
      DebuggerServer.removeGlobalActor(factory);
      activeGlobalActors.delete(factory);
    },

    
    addTabActor: function(factory, name) {
      DebuggerServer.addTabActor(factory, name);
      activeTabActors.add(factory);
    },
    
    removeTabActor: function(factory) {
      DebuggerServer.removeTabActor(factory);
      activeTabActors.delete(factory);
    },

    
    
    destroy: function() {
      for (let factory of activeTabActors) {
        DebuggerServer.removeTabActor(factory);
      }
      activeTabActors = null;
      for (let factory of activeGlobalActors) {
        DebuggerServer.removeGlobalActor(factory);
      }
      activeGlobalActors = null;
    }
  };
};




var DebuggerServer = {
  _listeners: [],
  _initialized: false,
  
  globalActorFactories: {},
  
  tabActorFactories: {},

  LONG_STRING_LENGTH: 10000,
  LONG_STRING_INITIAL_LENGTH: 1000,
  LONG_STRING_READ_LENGTH: 65 * 1024,

  




  chromeWindowType: null,

  


  allowChromeProcess: false,

  


  init: function DS_init() {
    if (this.initialized) {
      return;
    }

    this._connections = {};
    this._nextConnID = 0;

    this._initialized = true;
  },

  get protocol() require("devtools/server/protocol"),

  get initialized() this._initialized,

  






  destroy: function DS_destroy() {
    if (!this._initialized) {
      return;
    }

    for (let connID of Object.getOwnPropertyNames(this._connections)) {
      this._connections[connID].close();
    }

    for (let id of Object.getOwnPropertyNames(gRegisteredModules)) {
      this.unregisterModule(id);
    }
    gRegisteredModules = Object.create(null);

    this.closeAllListeners();
    this.globalActorFactories = {};
    this.tabActorFactories = {};
    this._initialized = false;

    dumpn("Debugger server is shut down.");
  },

  


  _checkInit: function DS_checkInit() {
    if (!this._initialized) {
      throw "DebuggerServer has not been initialized.";
    }

    if (!this.createRootActor) {
      throw "Use DebuggerServer.addActors() to add a root actor implementation.";
    }
  },

  







  addActors: function DS_addActors(aURL) {
    loadSubScript.call(this, aURL);
  },

  


































  registerModule: function(id, options) {
    if (id in gRegisteredModules) {
      throw new Error("Tried to register a module twice: " + id + "\n");
    }

    if (options) {
      
      let {prefix, constructor, type} = options;
      if (typeof(prefix) !== "string") {
        throw new Error("Lazy actor definition for '" + id + "' requires a string 'prefix' option.");
      }
      if (typeof(constructor) !== "string") {
        throw new Error("Lazy actor definition for '" + id + "' requires a string 'constructor' option.");
      }
      if (!("global" in type) && !("tab" in type)) {
        throw new Error("Lazy actor definition for '" + id + "' requires a dictionary 'type' option whose attributes can be 'global' or 'tab'.");
      }
      let name = prefix + "Actor";
      let mod = {
        id: id,
        prefix: prefix,
        constructorName: constructor,
        type: type,
        globalActor: type.global,
        tabActor: type.tab
      };
      gRegisteredModules[id] = mod;
      if (mod.tabActor) {
        this.addTabActor(mod, name);
      }
      if (mod.globalActor) {
        this.addGlobalActor(mod, name);
      }
    } else {
      
      let moduleAPI = ModuleAPI();
      let mod = require(id);
      mod.register(moduleAPI);
      gRegisteredModules[id] = {
        module: mod,
        api: moduleAPI
      };
    }
  },

  


  isModuleRegistered: function(id) {
    return (id in gRegisteredModules);
  },

  


  unregisterModule: function(id) {
    let mod = gRegisteredModules[id];
    if (!mod) {
      throw new Error("Tried to unregister a module that was not previously registered.");
    }

    
    if (mod.tabActor) {
      this.removeTabActor(mod);
    }
    if (mod.globalActor) {
      this.removeGlobalActor(mod);
    }

    if (mod.module) {
      
      mod.module.unregister(mod.api);
      mod.api.destroy();
    }

    delete gRegisteredModules[id];
  },

  









  addBrowserActors: function(aWindowType = "navigator:browser", restrictPrivileges = false) {
    this.chromeWindowType = aWindowType;
    this.registerModule("devtools/server/actors/webbrowser");

    if (!restrictPrivileges) {
      this.addTabActors();
      this.registerModule("devtools/server/actors/preference", {
        prefix: "preference",
        constructor: "PreferenceActor",
        type: { global: true }
      });
      this.registerModule("devtools/server/actors/actor-registry", {
        prefix: "actorRegistry",
        constructor: "ActorRegistryActor",
        type: { global: true }
      });
    }
    let win = Services.wm.getMostRecentWindow(DebuggerServer.chromeWindowType);
    if (win && win.navigator.mozSettings) {
      this.registerModule("devtools/server/actors/settings", {
        prefix: "settings",
        constructor: "SettingsActor",
        type: { global: true }
      });
    }
    this.registerModule("devtools/server/actors/webapps", {
      prefix: "webapps",
      constructor: "WebappsActor",
      type: { global: true }
    });
    this.registerModule("devtools/server/actors/device", {
      prefix: "device",
      constructor: "DeviceActor",
      type: { global: true }
    });
    this.registerModule("devtools/server/actors/director-registry", {
      prefix: "directorRegistry",
      constructor: "DirectorRegistryActor",
      type: { global: true }
    });
  },

  


  addChildActors: function () {
    
    
    
    if (!DebuggerServer.tabActorFactories.hasOwnProperty("consoleActor")) {
      this.addTabActors();
    }
    
    if (!this.isModuleRegistered("devtools/server/actors/webbrowser")) {
      this.registerModule("devtools/server/actors/webbrowser");
    }
    if (!("ContentActor" in this)) {
      this.addActors("resource://gre/modules/devtools/server/actors/childtab.js");
    }
  },

  


  addTabActors: function() {
    this.registerModule("devtools/server/actors/webconsole", {
      prefix: "console",
      constructor: "WebConsoleActor",
      type: { tab: true }
    });
    this.registerModule("devtools/server/actors/inspector", {
      prefix: "inspector",
      constructor: "InspectorActor",
      type: { tab: true }
    });
    this.registerModule("devtools/server/actors/call-watcher", {
      prefix: "callWatcher",
      constructor: "CallWatcherActor",
      type: { tab: true }
    });
    this.registerModule("devtools/server/actors/canvas", {
      prefix: "canvas",
      constructor: "CanvasActor",
      type: { tab: true }
    });
    this.registerModule("devtools/server/actors/webgl", {
      prefix: "webgl",
      constructor: "WebGLActor",
      type: { tab: true }
    });
    this.registerModule("devtools/server/actors/webaudio", {
      prefix: "webaudio",
      constructor: "WebAudioActor",
      type: { tab: true }
    });
    this.registerModule("devtools/server/actors/stylesheets", {
      prefix: "styleSheets",
      constructor: "StyleSheetsActor",
      type: { tab: true }
    });
    this.registerModule("devtools/server/actors/styleeditor", {
      prefix: "styleEditor",
      constructor: "StyleEditorActor",
      type: { tab: true }
    });
    this.registerModule("devtools/server/actors/storage", {
      prefix: "storage",
      constructor: "StorageActor",
      type: { tab: true }
    });
    this.registerModule("devtools/server/actors/gcli", {
      prefix: "gcli",
      constructor: "GcliActor",
      type: { tab: true }
    });
    this.registerModule("devtools/server/actors/tracer", {
      prefix: "trace",
      constructor: "TracerActor",
      type: { tab: true }
    });
    this.registerModule("devtools/server/actors/memory", {
      prefix: "memory",
      constructor: "MemoryActor",
      type: { tab: true }
    });
    this.registerModule("devtools/server/actors/framerate", {
      prefix: "framerate",
      constructor: "FramerateActor",
      type: { tab: true }
    });
    this.registerModule("devtools/server/actors/eventlooplag", {
      prefix: "eventLoopLag",
      constructor: "EventLoopLagActor",
      type: { tab: true }
    });
    this.registerModule("devtools/server/actors/layout", {
      prefix: "reflow",
      constructor: "ReflowActor",
      type: { tab: true }
    });
    this.registerModule("devtools/server/actors/csscoverage", {
      prefix: "cssUsage",
      constructor: "CSSUsageActor",
      type: { tab: true }
    });
    this.registerModule("devtools/server/actors/monitor", {
      prefix: "monitor",
      constructor: "MonitorActor",
      type: { tab: true }
    });
    this.registerModule("devtools/server/actors/timeline", {
      prefix: "timeline",
      constructor: "TimelineActor",
      type: { tab: true }
    });
    this.registerModule("devtools/server/actors/director-manager", {
      prefix: "directorManager",
      constructor: "DirectorManagerActor",
      type: { global: false, tab: true }
    });
    if ("nsIProfiler" in Ci) {
      this.registerModule("devtools/server/actors/profiler", {
        prefix: "profiler",
        constructor: "ProfilerActor",
        type: { tab: true }
      });
    }
    this.registerModule("devtools/server/actors/animation", {
      prefix: "animations",
      constructor: "AnimationsActor",
      type: { tab: true }
    });
  },

  








  setAddonOptions: function DS_setAddonOptions(aId, aOptions) {
    if (!this._initialized) {
      return;
    }

    let promises = [];

    
    for (let connID of Object.getOwnPropertyNames(this._connections)) {
      promises.push(this._connections[connID].setAddonOptions(aId, aOptions));
    }

    return all(promises);
  },

  get listeningSockets() {
    return this._listeners.length;
  },

  














  createListener: function() {
    if (!Services.prefs.getBoolPref("devtools.debugger.remote-enabled")) {
      throw new Error("Can't create listener, remote debugging disabled");
    }
    this._checkInit();
    return DebuggerSocket.createListener();
  },

  



  _addListener: function(listener) {
    this._listeners.push(listener);
  },

  



  _removeListener: function(listener) {
    this._listeners = this._listeners.filter(l => l !== listener);
  },

  





  closeAllListeners: function() {
    if (!this.listeningSockets) {
      return false;
    }

    for (let listener of this._listeners) {
      listener.close();
    }

    return true;
  },

  










  connectPipe: function DS_connectPipe(aPrefix) {
    this._checkInit();

    let serverTransport = new LocalDebuggerTransport;
    let clientTransport = new LocalDebuggerTransport(serverTransport);
    serverTransport.other = clientTransport;
    let connection = this._onConnection(serverTransport, aPrefix);

    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    clientTransport._serverConnection = connection;

    return clientTransport;
  },

  









  connectToParent: function(aPrefix, aMessageManager) {
    this._checkInit();

    let transport = new ChildDebuggerTransport(aMessageManager, aPrefix);
    return this._onConnection(transport, aPrefix, true);
  },

  connectToContent: function (aConnection, aMm) {
    let deferred = defer();

    let prefix = aConnection.allocID("content-process");
    let actor, childTransport;

    aMm.addMessageListener("debug:content-process-actor", function listener(msg) {
      
      
      aMm.removeMessageListener("debug:content-process-actor", listener);

      
      childTransport = new ChildDebuggerTransport(aMm, prefix);
      childTransport.hooks = {
        onPacket: aConnection.send.bind(aConnection),
        onClosed: function () {}
      };
      childTransport.ready();

      aConnection.setForwarding(prefix, childTransport);

      dumpn("establishing forwarding for process with prefix " + prefix);

      actor = msg.json.actor;

      deferred.resolve(actor);
    });

    aMm.sendAsyncMessage("DevTools:InitDebuggerServer", {
      prefix: prefix
    });

    function onClose() {
      Services.obs.removeObserver(onMessageManagerClose, "message-manager-close");
      events.off(aConnection, "closed", onClose);
      if (childTransport) {
        
        
        childTransport.close();
        childTransport = null;
        aConnection.cancelForwarding(prefix);

        
        try {
          aMm.sendAsyncMessage("debug:content-process-destroy");
        } catch(e) {}
      }
    }

    let onMessageManagerClose = DevToolsUtils.makeInfallible(function (subject, topic, data) {
      if (subject == aMm) {
        onClose();
        aConnection.send({ from: actor.actor, type: "tabDetached" });
      }
    }).bind(this);
    Services.obs.addObserver(onMessageManagerClose,
                             "message-manager-close", false);

    events.on(aConnection, "closed", onClose);

    return deferred.promise;
  },

  





  get isInChildProcess() !!this.parentMessageManager,

  









  setupInChild: function({ module, setupChild, args }) {
    if (this.isInChildProcess) {
      return;
    }

    const gMessageManager = Cc["@mozilla.org/globalmessagemanager;1"].
      getService(Ci.nsIMessageListenerManager);

    gMessageManager.broadcastAsyncMessage("debug:setup-in-child", {
      module: module,
      setupChild: setupChild,
      args: args,
    });
  },

  











  setupInParent: function({ module, setupParent }) {
    if (!this.isInChildProcess) {
      return false;
    }

    let { sendSyncMessage } = DebuggerServer.parentMessageManager;

    return sendSyncMessage("debug:setup-in-parent", {
      module: module,
      setupParent: setupParent
    });
  },

  














  connectToChild: function(aConnection, aFrame, aOnDestroy) {
    let deferred = defer();

    let mm = aFrame.QueryInterface(Ci.nsIFrameLoaderOwner).frameLoader
             .messageManager;
    mm.loadFrameScript("resource://gre/modules/devtools/server/child.js", false);

    let actor, childTransport;
    let prefix = aConnection.allocID("child");
    let netMonitor = null;

    
    
    let onSetupInParent = function (msg) {
      let { module, setupParent } = msg.json;
      let m, fn;

      try {
        m = require(module);

        if (!setupParent in m) {
          dumpn("ERROR: module '" + module + "' does not export '" + setupParent + "'");
          return false;
        }

        m[setupParent]({ mm: mm, prefix: prefix });

        return true;
      } catch(e) {
        let error_msg = "exception during actor module setup running in the parent process: ";
        DevToolsUtils.reportException(error_msg + e);
        dumpn("ERROR: " + error_msg + " \n\t module: '" + module + "' \n\t setupParent: '" + setupParent + "'\n" +
              DevToolsUtils.safeErrorString(e));
        return false;
      }
    };
    mm.addMessageListener("debug:setup-in-parent", onSetupInParent);

    let onActorCreated = DevToolsUtils.makeInfallible(function (msg) {
      if (msg.json.prefix != prefix) {
        return;
      }
      mm.removeMessageListener("debug:actor", onActorCreated);

      
      childTransport = new ChildDebuggerTransport(mm, prefix);
      childTransport.hooks = {
        onPacket: aConnection.send.bind(aConnection),
        onClosed: function () {}
      };
      childTransport.ready();

      aConnection.setForwarding(prefix, childTransport);

      dumpn("establishing forwarding for app with prefix " + prefix);

      actor = msg.json.actor;

      let { NetworkMonitorManager } = require("devtools/toolkit/webconsole/network-monitor");
      netMonitor = new NetworkMonitorManager(aFrame, actor.actor);

      events.emit(DebuggerServer, "new-child-process", { mm: mm });

      deferred.resolve(actor);
    }).bind(this);
    mm.addMessageListener("debug:actor", onActorCreated);

    let destroy = DevToolsUtils.makeInfallible(function () {
      
      
      DebuggerServer.emit("disconnected-from-child:" + prefix, { mm: mm, prefix: prefix });

      if (childTransport) {
        
        
        childTransport.close();
        childTransport = null;
        aConnection.cancelForwarding(prefix);

        
        mm.sendAsyncMessage("debug:disconnect", { prefix: prefix });
      } else {
        
        
        
        deferred.resolve(null);
      }
      if (actor) {
        
        
        
        aConnection.send({ from: actor.actor, type: "tabDetached" });
        actor = null;
      }

      if (netMonitor) {
        netMonitor.destroy();
        netMonitor = null;
      }

      if (aOnDestroy) {
        aOnDestroy(mm);
      }

      
      Services.obs.removeObserver(onMessageManagerClose, "message-manager-close");
      mm.removeMessageListener("debug:setup-in-parent", onSetupInParent);
      if (!actor) {
        mm.removeMessageListener("debug:actor", onActorCreated);
      }
      events.off(aConnection, "closed", destroy);
    });

    
    let onMessageManagerClose = function (subject, topic, data) {
      if (subject == mm) {
        destroy();
      }
    };
    Services.obs.addObserver(onMessageManagerClose,
                             "message-manager-close", false);

    
    
    events.on(aConnection, "closed", destroy);

    mm.sendAsyncMessage("debug:connect", { prefix: prefix });

    return deferred.promise;
  },

  









  _onConnection: function DS_onConnection(aTransport, aForwardingPrefix, aNoRootActor = false) {
    let connID;
    if (aForwardingPrefix) {
      connID = aForwardingPrefix + "/";
    } else {
      
      
      
      
      connID = "server" + loader.id + ".conn" + this._nextConnID++ + '.';
    }

    let conn = new DebuggerServerConnection(connID, aTransport);
    this._connections[connID] = conn;

    
    if (!aNoRootActor) {
      conn.rootActor = this.createRootActor(conn);
      if (aForwardingPrefix)
        conn.rootActor.actorID = aForwardingPrefix + "/root";
      else
        conn.rootActor.actorID = "root";
      conn.addActor(conn.rootActor);
      aTransport.send(conn.rootActor.sayHello());
    }
    aTransport.ready();

    this.emit("connectionchange", "opened", conn);
    return conn;
  },

  


  _connectionClosed: function DS_connectionClosed(aConnection) {
    delete this._connections[aConnection.prefix];
    this.emit("connectionchange", "closed", aConnection);
  },

  

  setRootActor: function DS_setRootActor(aFunction) {
    this.createRootActor = aFunction;
  },

  





















  addTabActor: function DS_addTabActor(aActor, aName) {
    let name = aName ? aName : aActor.prototype.actorPrefix;
    if (["title", "url", "actor"].indexOf(name) != -1) {
      throw Error(name + " is not allowed");
    }
    if (DebuggerServer.tabActorFactories.hasOwnProperty(name)) {
      throw Error(name + " already exists");
    }
    DebuggerServer.tabActorFactories[name] = new RegisteredActorFactory(aActor, name);
  },

  











  removeTabActor: function DS_removeTabActor(aActor) {
    for (let name in DebuggerServer.tabActorFactories) {
      let handler = DebuggerServer.tabActorFactories[name];
      if ((handler.name && handler.name == aActor.name) ||
          (handler.id && handler.id == aActor.id)) {
        delete DebuggerServer.tabActorFactories[name];
        for (let connID of Object.getOwnPropertyNames(this._connections)) {
          this._connections[connID].rootActor.removeActorByName(name);
        }
      }
    }
  },

  






















  addGlobalActor: function DS_addGlobalActor(aActor, aName) {
    let name = aName ? aName : aActor.prototype.actorPrefix;
    if (["from", "tabs", "selected"].indexOf(name) != -1) {
      throw Error(name + " is not allowed");
    }
    if (DebuggerServer.globalActorFactories.hasOwnProperty(name)) {
      throw Error(name + " already exists");
    }
    DebuggerServer.globalActorFactories[name] = new RegisteredActorFactory(aActor, name);
  },

  











  removeGlobalActor: function DS_removeGlobalActor(aActor) {
    for (let name in DebuggerServer.globalActorFactories) {
      let handler = DebuggerServer.globalActorFactories[name];
      if ((handler.name && handler.name == aActor.name) ||
          (handler.id && handler.id == aActor.id)) {
        delete DebuggerServer.globalActorFactories[name];
        for (let connID of Object.getOwnPropertyNames(this._connections)) {
          this._connections[connID].rootActor.removeActorByName(name);
        }
      }
    }
  }
};


DevToolsUtils.defineLazyGetter(DebuggerServer, "Authenticators", () => {
  return Authentication.Authenticators;
});
DevToolsUtils.defineLazyGetter(DebuggerServer, "AuthenticationResult", () => {
  return Authentication.AuthenticationResult;
});

EventEmitter.decorate(DebuggerServer);

if (this.exports) {
  exports.DebuggerServer = DebuggerServer;
  exports.ActorPool = ActorPool;
  exports.OriginalLocation = OriginalLocation;
}


this.DebuggerServer = DebuggerServer;
this.ActorPool = ActorPool;
this.OriginalLocation = OriginalLocation;




let includes = ["Components", "Ci", "Cu", "require", "Services", "DebuggerServer",
                "ActorPool", "DevToolsUtils"];
includes.forEach(name => {
  DebuggerServer[name] = this[name];
});














function DebuggerServerConnection(aPrefix, aTransport)
{
  this._prefix = aPrefix;
  this._transport = aTransport;
  this._transport.hooks = this;
  this._nextID = 1;

  this._actorPool = new ActorPool(this);
  this._extraPools = [this._actorPool];

  
  
  
  
  
  
  
  this._actorResponses = new Map;

  





  this._forwardingPrefixes = new Map;
}

DebuggerServerConnection.prototype = {
  _prefix: null,
  get prefix() { return this._prefix },

  _transport: null,
  get transport() { return this._transport },

  close: function() {
    this._transport.close();
  },

  send: function DSC_send(aPacket) {
    this.transport.send(aPacket);
  },

  



  startBulkSend: function(header) {
    return this.transport.startBulkSend(header);
  },

  allocID: function DSC_allocID(aPrefix) {
    return this.prefix + (aPrefix || '') + this._nextID++;
  },

  


  addActorPool: function DSC_addActorPool(aActorPool) {
    this._extraPools.push(aActorPool);
  },

  








  removeActorPool: function DSC_removeActorPool(aActorPool, aNoCleanup) {
    let index = this._extraPools.lastIndexOf(aActorPool);
    if (index > -1) {
      let pool = this._extraPools.splice(index, 1);
      if (!aNoCleanup) {
        pool.map(function(p) { p.cleanup(); });
      }
    }
  },

  


  addActor: function DSC_addActor(aActor) {
    this._actorPool.addActor(aActor);
  },

  


  removeActor: function DSC_removeActor(aActor) {
    this._actorPool.removeActor(aActor);
  },

  


  unmanage: function(aActor) {
    return this.removeActor(aActor);
  },

  






  getActor: function DSC_getActor(aActorID) {
    let pool = this.poolFor(aActorID);
    if (pool) {
      return pool.get(aActorID);
    }

    if (aActorID === "root") {
      return this.rootActor;
    }

    return null;
  },

  _getOrCreateActor: function(actorID) {
    let actor = this.getActor(actorID);
    if (!actor) {
      this.transport.send({ from: actorID ? actorID : "root",
                            error: "noSuchActor",
                            message: "No such actor for ID: " + actorID });
      return;
    }

    
    if (actor instanceof ObservedActorFactory) {
      try {
        actor= actor.createActor();
      } catch (e) {
        this.transport.send(this._unknownError(
          "Error occurred while creating actor '" + actor.name,
          e));
      }
    } else if (typeof(actor) !== "object") {
      
      
      throw new Error("Unexpected actor constructor/function in ActorPool " +
                      "for actorID=" + actorID + ".");
    }

    return actor;
  },

  poolFor: function DSC_actorPool(aActorID) {
    for (let pool of this._extraPools) {
      if (pool.has(aActorID)) {
        return pool;
      }
    }
    return null;
  },

  _unknownError: function DSC__unknownError(aPrefix, aError) {
    let errorString = aPrefix + ": " + DevToolsUtils.safeErrorString(aError);
    reportError(errorString);
    dumpn(errorString);
    return {
      error: "unknownError",
      message: errorString
    };
  },

  _queueResponse: function(from, type, response) {
    let pendingResponse = this._actorResponses.get(from) || resolve(null);
    let responsePromise = pendingResponse.then(() => {
      return response;
    }).then(aResponse => {
      if (!aResponse.from) {
        aResponse.from = from;
      }
      this.transport.send(aResponse);
    }).then(null, (e) => {
      let errorPacket = this._unknownError(
        "error occurred while processing '" + type,
        e);
      errorPacket.from = from;
      this.transport.send(errorPacket);
    });

    this._actorResponses.set(from, responsePromise);
  },

  








  setAddonOptions: function DSC_setAddonOptions(aId, aOptions) {
    let addonList = this.rootActor._parameters.addonList;
    if (!addonList) {
      return resolve();
    }
    return addonList.getList().then((addonActors) => {
      for (let actor of addonActors) {
        if (actor.id != aId) {
          continue;
        }
        actor.setOptions(aOptions);
        return;
      }
    });
  },

  

  














  setForwarding: function(aPrefix, aTransport) {
    this._forwardingPrefixes.set(aPrefix, aTransport);
  },

  



  cancelForwarding: function(aPrefix) {
    this._forwardingPrefixes.delete(aPrefix);
  },

  sendActorEvent: function (actorID, eventName, event = {}) {
    event.from = actorID;
    event.type = eventName;
    this.send(event);
  },

  

  





  onPacket: function DSC_onPacket(aPacket) {
    
    
    
    
    
    
    if (this._forwardingPrefixes.size > 0) {
      let separator = aPacket.to.indexOf('/');
      if (separator >= 0) {
        let forwardTo = this._forwardingPrefixes.get(aPacket.to.substring(0, separator));
        if (forwardTo) {
          forwardTo.send(aPacket);
          return;
        }
      }
    }

    let actor = this._getOrCreateActor(aPacket.to);
    if (!actor) {
      return;
    }

    var ret = null;

    
    if (aPacket.type == "requestTypes") {
      ret = { from: actor.actorID, requestTypes: Object.keys(actor.requestTypes) };
    } else if (actor.requestTypes && actor.requestTypes[aPacket.type]) {
      
      try {
        this.currentPacket = aPacket;
        ret = actor.requestTypes[aPacket.type].bind(actor)(aPacket, this);
      } catch(e) {
        this.transport.send(this._unknownError(
          "error occurred while processing '" + aPacket.type,
          e));
      } finally {
        this.currentPacket = undefined;
      }
    } else {
      ret = { error: "unrecognizedPacketType",
              message: ("Actor " + actor.actorID +
                        " does not recognize the packet type " +
                        aPacket.type) };
    }

    
    if (ret) {
      this._queueResponse(aPacket.to, aPacket.type, ret);
    }
  },

  





























  onBulkPacket: function(packet) {
    let { actor: actorKey, type, length } = packet;

    let actor = this._getOrCreateActor(actorKey);
    if (!actor) {
      return;
    }

    
    let ret;
    if (actor.requestTypes && actor.requestTypes[type]) {
      try {
        ret = actor.requestTypes[type].call(actor, packet);
      } catch(e) {
        this.transport.send(this._unknownError(
          "error occurred while processing bulk packet '" + type, e));
        packet.done.reject(e);
      }
    } else {
      let message = "Actor " + actorKey +
                    " does not recognize the bulk packet type " + type;
      ret = { error: "unrecognizedPacketType",
              message: message };
      packet.done.reject(new Error(message));
    }

    
    if (ret) {
      this._queueResponse(actorKey, type, ret);
    }
  },

  






  onClosed: function DSC_onClosed(aStatus) {
    dumpn("Cleaning up connection.");
    if (!this._actorPool) {
      
      return;
    }
    events.emit(this, "closed", aStatus);

    this._actorPool = null;
    this._extraPools.map(function(p) { p.cleanup(); });
    this._extraPools = null;

    DebuggerServer._connectionClosed(this);
  },

  


  _dumpPools: function DSC_dumpPools() {
    dumpn("/-------------------- dumping pools:");
    if (this._actorPool) {
      dumpn("--------------------- actorPool actors: " +
            uneval(Object.keys(this._actorPool._actors)));
    }
    for each (let pool in this._extraPools) {
      if (pool !== this._actorPool) {
        dumpn("--------------------- extraPool actors: " +
              uneval(Object.keys(pool._actors)));
      }
    }
  },

  


  _dumpPool: function DSC_dumpPools(aPool) {
    dumpn("/-------------------- dumping pool:");
    dumpn("--------------------- actorPool actors: " +
          uneval(Object.keys(aPool._actors)));
  }
};
