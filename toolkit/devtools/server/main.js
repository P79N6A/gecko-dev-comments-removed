





"use strict";





let { Ci, Cc, CC, Cu, Cr } = require("chrome");
let Services = require("Services");
let { ActorPool } = require("devtools/server/actors/common");
let { DebuggerTransport, LocalDebuggerTransport, ChildDebuggerTransport } =
  require("devtools/toolkit/transport/transport");
let DevToolsUtils = require("devtools/toolkit/DevToolsUtils");
let { dumpn, dumpv, dbg_assert } = DevToolsUtils;
let Services = require("Services");
let EventEmitter = require("devtools/toolkit/event-emitter");
let Debugger = require("Debugger");



var { Ci, Cc, CC, Cu, Cr } = require("chrome");



this.Ci = Ci;
this.Cc = Cc;
this.CC = CC;
this.Cu = Cu;
this.Cr = Cr;
this.Debugger = Debugger;
this.Services = Services;
this.ActorPool = ActorPool;
this.DevToolsUtils = DevToolsUtils;
this.dumpn = dumpn;
this.dumpv = dumpv;
this.dbg_assert = dbg_assert;



Object.defineProperty(this, "Components", {
  get: function () require("chrome").components
});

const DBG_STRINGS_URI = "chrome://global/locale/devtools/debugger.properties";

DevToolsUtils.defineLazyGetter(this, "nsFile", () => {
  return CC("@mozilla.org/file/local;1", "nsIFile", "initWithPath");
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

let events = require("sdk/event/core");
let {defer, resolve, reject, all} = require("devtools/toolkit/deprecated-sync-thenables");
this.defer = defer;
this.resolve = resolve;
this.reject = reject;
this.all = all;


DevToolsUtils.defineLazyGetter(this, "ServerSocket", () => {
  return CC("@mozilla.org/network/server-socket;1",
            "nsIServerSocket",
            "initSpecialConnection");
});

DevToolsUtils.defineLazyGetter(this, "UnixDomainServerSocket", () => {
  return CC("@mozilla.org/network/server-socket;1",
            "nsIServerSocket",
            "initWithFilename");
});

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
  }
};




var DebuggerServer = {
  _listeners: [],
  _initialized: false,
  _transportInitialized: false,
  
  globalActorFactories: {},
  
  tabActorFactories: {},

  LONG_STRING_LENGTH: 10000,
  LONG_STRING_INITIAL_LENGTH: 1000,
  LONG_STRING_READ_LENGTH: 65 * 1024,

  



  _allowConnection: null,

  




  chromeWindowType: null,

  






  _defaultAllowConnection: function DS__defaultAllowConnection() {
    let bundle = Services.strings.createBundle(DBG_STRINGS_URI)
    let title = bundle.GetStringFromName("remoteIncomingPromptTitle");
    let msg = bundle.GetStringFromName("remoteIncomingPromptMessage");
    let disableButton = bundle.GetStringFromName("remoteIncomingPromptDisable");
    let prompt = Services.prompt;
    let flags = prompt.BUTTON_POS_0 * prompt.BUTTON_TITLE_OK +
                prompt.BUTTON_POS_1 * prompt.BUTTON_TITLE_CANCEL +
                prompt.BUTTON_POS_2 * prompt.BUTTON_TITLE_IS_STRING +
                prompt.BUTTON_POS_1_DEFAULT;
    let result = prompt.confirmEx(null, title, msg, flags, null, null,
                                  disableButton, null, { value: false });
    if (result == 0) {
      return true;
    }
    if (result == 2) {
      DebuggerServer.closeAllListeners();
      Services.prefs.setBoolPref("devtools.debugger.remote-enabled", false);
    }
    return false;
  },

  






  init: function DS_init(aAllowConnectionCallback) {
    if (this.initialized) {
      return;
    }

    this.initTransport(aAllowConnectionCallback);

    this._initialized = true;
  },

  protocol: require("devtools/server/protocol"),

  







  initTransport: function DS_initTransport(aAllowConnectionCallback) {
    if (this._transportInitialized) {
      return;
    }

    this._connections = {};
    this._nextConnID = 0;
    this._transportInitialized = true;
    this._allowConnection = aAllowConnectionCallback ?
                            aAllowConnectionCallback :
                            this._defaultAllowConnection;
  },

  get initialized() this._initialized,

  






  destroy: function DS_destroy() {
    if (!this._initialized) {
      return;
    }

    for (let connID of Object.getOwnPropertyNames(this._connections)) {
      this._connections[connID].close();
    }

    for (let id of Object.getOwnPropertyNames(gRegisteredModules)) {
      let mod = gRegisteredModules[id];
      mod.module.unregister(mod.api);
    }
    gRegisteredModules = {};

    this.closeAllListeners();
    this.globalActorFactories = {};
    this.tabActorFactories = {};
    this._allowConnection = null;
    this._transportInitialized = false;
    this._initialized = false;

    dumpn("Debugger server is shut down.");
  },

  







  addActors: function DS_addActors(aURL) {
    loadSubScript.call(this, aURL);
  },

  





  registerModule: function(id) {
    if (id in gRegisteredModules) {
      throw new Error("Tried to register a module twice: " + id + "\n");
    }

    let moduleAPI = ModuleAPI();
    let mod = require(id);
    mod.register(moduleAPI);
    gRegisteredModules[id] = { module: mod, api: moduleAPI };
  },

  


  isModuleRegistered: function(id) {
    return (id in gRegisteredModules);
  },

  


  unregisterModule: function(id) {
    let mod = gRegisteredModules[id];
    if (!mod) {
      throw new Error("Tried to unregister a module that was not previously registered.");
    }
    mod.module.unregister(mod.api);
    mod.api.destroy();
    delete gRegisteredModules[id];
  },

  









  addBrowserActors: function(aWindowType = "navigator:browser", restrictPrivileges = false) {
    this.chromeWindowType = aWindowType;
    this.registerModule("devtools/server/actors/webbrowser");

    if (!restrictPrivileges) {
      this.addTabActors();
      let { ChromeDebuggerActor } = require("devtools/server/actors/script");
      this.addGlobalActor(ChromeDebuggerActor, "chromeDebugger");
      this.registerModule("devtools/server/actors/preference");
    }

    this.addActors("resource://gre/modules/devtools/server/actors/webapps.js");
    this.registerModule("devtools/server/actors/device");
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
    this.registerModule("devtools/server/actors/script");
    this.registerModule("devtools/server/actors/webconsole");
    this.registerModule("devtools/server/actors/inspector");
    this.registerModule("devtools/server/actors/call-watcher");
    this.registerModule("devtools/server/actors/canvas");
    this.registerModule("devtools/server/actors/webgl");
    this.registerModule("devtools/server/actors/webaudio");
    this.registerModule("devtools/server/actors/stylesheets");
    this.registerModule("devtools/server/actors/styleeditor");
    this.registerModule("devtools/server/actors/storage");
    this.registerModule("devtools/server/actors/gcli");
    this.registerModule("devtools/server/actors/tracer");
    this.registerModule("devtools/server/actors/memory");
    this.registerModule("devtools/server/actors/framerate");
    this.registerModule("devtools/server/actors/eventlooplag");
    this.registerModule("devtools/server/actors/layout");
    this.registerModule("devtools/server/actors/csscoverage");
    this.registerModule("devtools/server/actors/monitor");
    if ("nsIProfiler" in Ci) {
      this.addActors("resource://gre/modules/devtools/server/actors/profiler.js");
    }
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

  












  openListener: function(portOrPath) {
    if (!Services.prefs.getBoolPref("devtools.debugger.remote-enabled")) {
      return;
    }
    this._checkInit();

    let listener = new SocketListener(this);
    listener.open(portOrPath);
    this._listeners.push(listener);
    return listener;
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

  












  connectToChild: function(aConnection, aFrame, aOnDisconnect) {
    let deferred = defer();

    let mm = aFrame.QueryInterface(Ci.nsIFrameLoaderOwner).frameLoader
             .messageManager;
    mm.loadFrameScript("resource://gre/modules/devtools/server/child.js", false);

    let actor, childTransport;
    let prefix = aConnection.allocID("child");
    let netMonitor = null;

    let onActorCreated = DevToolsUtils.makeInfallible(function (msg) {
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

      deferred.resolve(actor);
    }).bind(this);
    mm.addMessageListener("debug:actor", onActorCreated);

    let onMessageManagerDisconnect = DevToolsUtils.makeInfallible(function (subject, topic, data) {
      if (subject == mm) {
        Services.obs.removeObserver(onMessageManagerDisconnect, topic);
        if (childTransport) {
          
          
          childTransport.close();
          childTransport = null;
          aConnection.cancelForwarding(prefix);

          
          mm.sendAsyncMessage("debug:disconnect");
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

        if (aOnDisconnect) {
          aOnDisconnect(mm);
        }
      }
    }).bind(this);
    Services.obs.addObserver(onMessageManagerDisconnect,
                             "message-manager-disconnect", false);

    events.once(aConnection, "closed", () => {
      if (childTransport) {
        
        
        childTransport.close();
        childTransport = null;
        aConnection.cancelForwarding(prefix);

        
        mm.sendAsyncMessage("debug:disconnect");
      }
    });

    mm.sendAsyncMessage("debug:connect", { prefix: prefix });

    return deferred.promise;
  },

  


  _checkInit: function DS_checkInit() {
    if (!this._transportInitialized) {
      throw "DebuggerServer has not been initialized.";
    }

    if (!this.createRootActor) {
      throw "Use DebuggerServer.addActors() to add a root actor implementation.";
    }
  },

  









  _onConnection: function DS_onConnection(aTransport, aForwardingPrefix, aNoRootActor = false) {
    let connID;
    if (aForwardingPrefix) {
      connID = aForwardingPrefix + "/";
    } else {
      connID = "conn" + this._nextConnID++ + '.';
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

  
















  addTabActor: function DS_addTabActor(aFunction, aName) {
    let name = aName ? aName : aFunction.prototype.actorPrefix;
    if (["title", "url", "actor"].indexOf(name) != -1) {
      throw Error(name + " is not allowed");
    }
    if (DebuggerServer.tabActorFactories.hasOwnProperty(name)) {
      throw Error(name + " already exists");
    }
    DebuggerServer.tabActorFactories[name] = aFunction;
  },

  






  removeTabActor: function DS_removeTabActor(aFunction) {
    for (let name in DebuggerServer.tabActorFactories) {
      let handler = DebuggerServer.tabActorFactories[name];
      if (handler.name == aFunction.name) {
        delete DebuggerServer.tabActorFactories[name];
      }
    }
  },

  

















  addGlobalActor: function DS_addGlobalActor(aFunction, aName) {
    let name = aName ? aName : aFunction.prototype.actorPrefix;
    if (["from", "tabs", "selected"].indexOf(name) != -1) {
      throw Error(name + " is not allowed");
    }
    if (DebuggerServer.globalActorFactories.hasOwnProperty(name)) {
      throw Error(name + " already exists");
    }
    DebuggerServer.globalActorFactories[name] = aFunction;
  },

  






  removeGlobalActor: function DS_removeGlobalActor(aFunction) {
    for (let name in DebuggerServer.globalActorFactories) {
      let handler = DebuggerServer.globalActorFactories[name];
      if (handler.name == aFunction.name) {
        delete DebuggerServer.globalActorFactories[name];
      }
    }
  }
};

EventEmitter.decorate(DebuggerServer);

if (this.exports) {
  exports.DebuggerServer = DebuggerServer;
}

this.DebuggerServer = DebuggerServer;




let includes = ["Components", "Ci", "Cu", "require", "Services", "DebuggerServer",
                "ActorPool", "DevToolsUtils"];
includes.forEach(name => {
  DebuggerServer[name] = this[name];
});


if (this.exports) {
  exports.ActorPool = ActorPool;
}






function SocketListener(server) {
  this._server = server;
}

SocketListener.prototype = {

  






  open: function(portOrPath) {
    let flags = Ci.nsIServerSocket.KeepWhenOffline;
    
    if (Services.prefs.getBoolPref("devtools.debugger.force-local")) {
      flags |= Ci.nsIServerSocket.LoopbackOnly;
    }

    try {
      let backlog = 4;
      let port = Number(portOrPath);
      if (port) {
        this._socket = new ServerSocket(port, flags, backlog);
      } else {
        let file = nsFile(portOrPath);
        if (file.exists())
          file.remove(false);
        this._socket = new UnixDomainServerSocket(file, parseInt("666", 8),
                                                  backlog);
      }
      this._socket.asyncListen(this);
    } catch (e) {
      dumpn("Could not start debugging listener on '" + portOrPath + "': " + e);
      throw Cr.NS_ERROR_NOT_AVAILABLE;
    }
  },

  



  close: function() {
    this._socket.close();
    this._server._removeListener(this);
    this._server = null;
  },

  



  get port() {
    if (!this._socket) {
      return null;
    }
    return this._socket.port;
  },

  

  onSocketAccepted:
  DevToolsUtils.makeInfallible(function(aSocket, aTransport) {
    if (Services.prefs.getBoolPref("devtools.debugger.prompt-connection") &&
        !this._server._allowConnection()) {
      return;
    }
    dumpn("New debugging connection on " +
          aTransport.host + ":" + aTransport.port);

    let input = aTransport.openInputStream(0, 0, 0);
    let output = aTransport.openOutputStream(0, 0, 0);
    let transport = new DebuggerTransport(input, output);
    this._server._onConnection(transport);
  }, "SocketListener.onSocketAccepted"),

  onStopListening: function(aSocket, status) {
    dumpn("onStopListening, status: " + status);
  }

};














function DebuggerServerConnection(aPrefix, aTransport)
{
  this._prefix = aPrefix;
  this._transport = aTransport;
  this._transport.hooks = this;
  this._nextID = 1;

  this._actorPool = new ActorPool(this);
  this._extraPools = [];

  
  
  
  
  
  
  
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

    
    if (typeof actor == "function") {
      let instance;
      try {
        instance = new actor();
      } catch (e) {
        this.transport.send(this._unknownError(
          "Error occurred while creating actor '" + actor.name,
          e));
      }
      instance.parentID = actor.parentID;
      
      
      
      instance.actorID = actor.actorID;
      actor.registeredPool.addActor(instance);
      actor = instance;
    }

    return actor;
  },

  poolFor: function DSC_actorPool(aActorID) {
    if (this._actorPool && this._actorPool.has(aActorID)) {
      return this._actorPool;
    }

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

    this._actorPool.cleanup();
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
    for each (let pool in this._extraPools)
      dumpn("--------------------- extraPool actors: " +
            uneval(Object.keys(pool._actors)));
  },

  


  _dumpPool: function DSC_dumpPools(aPool) {
    dumpn("/-------------------- dumping pool:");
    dumpn("--------------------- actorPool actors: " +
          uneval(Object.keys(aPool._actors)));
  }
};
