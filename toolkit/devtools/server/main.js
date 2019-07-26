





"use strict";









var Ci, Cc, CC, Cu, Cr, Components;
if (this.require) {
  ({ Ci, Cc, CC, Cu, Cr, components: Components }) = require("chrome");
} else {
  ({
    interfaces: Ci,
    classes: Cc,
    Constructor: CC,
    utils: Cu,
    results: Cr
  }) = Components;
}







var localRequire;
if (this.require) {
  localRequire = id => require(id);
} else {
  let { devtools } = Cu.import("resource://gre/modules/devtools/Loader.jsm", {});
  localRequire = id => devtools.require(id);
}

const DBG_STRINGS_URI = "chrome://global/locale/devtools/debugger.properties";

const nsFile = CC("@mozilla.org/file/local;1", "nsIFile", "initWithPath");
Cu.import("resource://gre/modules/reflect.jsm");
Cu.import("resource://gre/modules/devtools/DevToolsUtils.jsm");
Cu.import("resource://gre/modules/Services.jsm");
Cu.import("resource://gre/modules/XPCOMUtils.jsm");
let wantLogging = Services.prefs.getBoolPref("devtools.debugger.log");
const promptConnections = Services.prefs.getBoolPref("devtools.debugger.prompt-connection");

Cu.import("resource://gre/modules/jsdebugger.jsm");
addDebuggerToGlobal(this);

function loadSubScript(aURL)
{
  try {
    let loader = Cc["@mozilla.org/moz/jssubscript-loader;1"]
      .getService(Ci.mozIJSSubScriptLoader);
    loader.loadSubScript(aURL, this);
  } catch(e) {
    let errorStr = "Error loading: " + aURL + ": " + e + " - " + e.stack + "\n";
    dump(errorStr);
    Cu.reportError(errorStr);
    throw e;
  }
}

let loaderRequire = this.require;
this.require = null;
loadSubScript.call(this, "resource://gre/modules/commonjs/sdk/core/promise.js");
this.require = loaderRequire;

Cu.import("resource://gre/modules/devtools/SourceMap.jsm");
const escodegen = localRequire("escodegen/escodegen");

loadSubScript.call(this, "resource://gre/modules/devtools/DevToolsUtils.js");

function dumpn(str) {
  if (wantLogging) {
    for (let line of str.split(/\n/g)) {
      dump("DBG-SERVER: " + line + "\n");
    }
  }
}

function dbg_assert(cond, e) {
  if (!cond) {
    return e;
  }
}

loadSubScript.call(this, "resource://gre/modules/devtools/server/transport.js");


const ServerSocket = CC("@mozilla.org/network/server-socket;1",
                        "nsIServerSocket",
                        "initSpecialConnection");
const UnixDomainServerSocket = CC("@mozilla.org/network/server-socket;1",
                                  "nsIServerSocket",
                                  "initWithFilename");

var gRegisteredModules = Object.create(null);









function ModuleAPI() {
  let activeTabActors = new Set();
  let activeGlobalActors = new Set();

  return {
    
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
  _listener: null,
  _initialized: false,
  _transportInitialized: false,
  xpcInspector: null,
  
  _socketConnections: 0,
  
  globalActorFactories: {},
  
  tabActorFactories: {},

  LONG_STRING_LENGTH: 10000,
  LONG_STRING_INITIAL_LENGTH: 1000,
  LONG_STRING_READ_LENGTH: 1000,

  



  _allowConnection: null,

  




  chromeWindowType: null,

  



  onConnectionChange: null,

  _fireConnectionChange: function(aWhat) {
    if (this.onConnectionChange &&
        typeof this.onConnectionChange === "function") {
      this.onConnectionChange(aWhat);
    }
  },

  






  _defaultAllowConnection: function DS__defaultAllowConnection() {
    let title = L10N.getStr("remoteIncomingPromptTitle");
    let msg = L10N.getStr("remoteIncomingPromptMessage");
    let disableButton = L10N.getStr("remoteIncomingPromptDisable");
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
      DebuggerServer.closeListener(true);
      Services.prefs.setBoolPref("devtools.debugger.remote-enabled", false);
    }
    return false;
  },

  






  init: function DS_init(aAllowConnectionCallback) {
    if (this.initialized) {
      return;
    }

    this.xpcInspector = Cc["@mozilla.org/jsinspector;1"].getService(Ci.nsIJSInspector);
    this.initTransport(aAllowConnectionCallback);
    this.addActors("resource://gre/modules/devtools/server/actors/root.js");

    this._initialized = true;
  },

  







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

    this.closeListener();
    this.globalActorFactories = {};
    this.tabActorFactories = {};
    delete this._allowConnection;
    this._transportInitialized = false;
    this._initialized = false;

    this._fireConnectionChange("closed");

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
    let mod = localRequire(id);
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

  


  addBrowserActors: function(aWindowType) {
    this.chromeWindowType = aWindowType ? aWindowType : "navigator:browser";
    this.addActors("resource://gre/modules/devtools/server/actors/webbrowser.js");
    this.addActors("resource://gre/modules/devtools/server/actors/script.js");
    this.addGlobalActor(this.ChromeDebuggerActor, "chromeDebugger");
    this.addActors("resource://gre/modules/devtools/server/actors/webconsole.js");
    this.addActors("resource://gre/modules/devtools/server/actors/gcli.js");
    if ("nsIProfiler" in Ci)
      this.addActors("resource://gre/modules/devtools/server/actors/profiler.js");

    this.addActors("resource://gre/modules/devtools/server/actors/styleeditor.js");
    this.addActors("resource://gre/modules/devtools/server/actors/webapps.js");
    this.registerModule("devtools/server/actors/inspector");
    this.registerModule("devtools/server/actors/tracer");
    this.registerModule("devtools/server/actors/device");
  },

  


  addChildActors: function () {
    
    
    
    if (!("BrowserTabActor" in this)) {
      this.addActors("resource://gre/modules/devtools/server/actors/webbrowser.js");
      this.addActors("resource://gre/modules/devtools/server/actors/script.js");
      this.addActors("resource://gre/modules/devtools/server/actors/webconsole.js");
      this.addActors("resource://gre/modules/devtools/server/actors/gcli.js");
      this.addActors("resource://gre/modules/devtools/server/actors/styleeditor.js");
      this.registerModule("devtools/server/actors/inspector");
    }
    if (!("ContentAppActor" in DebuggerServer)) {
      this.addActors("resource://gre/modules/devtools/server/actors/childtab.js");
    }
  },

  






  openListener: function DS_openListener(aPortOrPath) {
    if (!Services.prefs.getBoolPref("devtools.debugger.remote-enabled")) {
      return false;
    }
    this._checkInit();

    
    if (this._listener) {
      return true;
    }

    let flags = Ci.nsIServerSocket.KeepWhenOffline;
    
    if (Services.prefs.getBoolPref("devtools.debugger.force-local")) {
      flags |= Ci.nsIServerSocket.LoopbackOnly;
    }

    try {
      let backlog = 4;
      let socket;
      let port = Number(aPortOrPath);
      if (port) {
        socket = new ServerSocket(port, flags, backlog);
      } else {
        let file = nsFile(aPortOrPath);
        if (file.exists())
          file.remove(false);
        socket = new UnixDomainServerSocket(file, parseInt("600", 8), backlog);
      }
      socket.asyncListen(this);
      this._listener = socket;
    } catch (e) {
      dumpn("Could not start debugging listener on '" + aPortOrPath + "': " + e);
      throw Cr.NS_ERROR_NOT_AVAILABLE;
    }
    this._socketConnections++;

    return true;
  },

  






  closeListener: function DS_closeListener(aForce) {
    if (!this._listener || this._socketConnections == 0) {
      return false;
    }

    
    
    if (--this._socketConnections == 0 || aForce) {
      this._listener.close();
      this._listener = null;
      this._socketConnections = 0;
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

  

  onSocketAccepted:
  makeInfallible(function DS_onSocketAccepted(aSocket, aTransport) {
    if (promptConnections && !this._allowConnection()) {
      return;
    }
    dumpn("New debugging connection on " + aTransport.host + ":" + aTransport.port);

    let input = aTransport.openInputStream(0, 0, 0);
    let output = aTransport.openOutputStream(0, 0, 0);
    let transport = new DebuggerTransport(input, output);
    DebuggerServer._onConnection(transport);
  }, "DebuggerServer.onSocketAccepted"),

  onStopListening: function DS_onStopListening(aSocket, status) {
    dumpn("onStopListening, status: " + status);
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
      connID = aForwardingPrefix + ":";
    } else {
      connID = "conn" + this._nextConnID++ + '.';
    }
    let conn = new DebuggerServerConnection(connID, aTransport);
    this._connections[connID] = conn;

    
    if (!aNoRootActor) {
      conn.rootActor = this.createRootActor(conn);
      if (aForwardingPrefix)
        conn.rootActor.actorID = aForwardingPrefix + ":root";
      else
        conn.rootActor.actorID = "root";
      conn.addActor(conn.rootActor);
      aTransport.send(conn.rootActor.sayHello());
    }
    aTransport.ready();

    this._fireConnectionChange("opened");
    return conn;
  },

  


  _connectionClosed: function DS_connectionClosed(aConnection) {
    delete this._connections[aConnection.prefix];
    this._fireConnectionChange("closed");
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

if (this.exports) {
  exports.DebuggerServer = DebuggerServer;
}








function ActorPool(aConnection)
{
  this.conn = aConnection;
  this._cleanups = {};
  this._actors = {};
}

if (this.exports) {
  exports.ActorPool = ActorPool;
}

ActorPool.prototype = {
  








  addActor: function AP_addActor(aActor) {
    aActor.conn = this.conn;
    if (!aActor.actorID) {
      let prefix = aActor.actorPrefix;
      if (typeof aActor == "function") {
        prefix = aActor.prototype.actorPrefix;
      }
      aActor.actorID = this.conn.allocID(prefix || undefined);
    }

    if (aActor.registeredPool) {
      aActor.registeredPool.removeActor(aActor);
    }
    aActor.registeredPool = this;

    this._actors[aActor.actorID] = aActor;
    if (aActor.disconnect) {
      this._cleanups[aActor.actorID] = aActor;
    }
  },

  get: function AP_get(aActorID) {
    return this._actors[aActorID];
  },

  has: function AP_has(aActorID) {
    return aActorID in this._actors;
  },

  


  isEmpty: function AP_isEmpty() {
    return Object.keys(this._actors).length == 0;
  },

  


  removeActor: function AP_remove(aActor) {
    delete this._actors[aActor.actorID];
    delete this._cleanups[aActor.actorID];
  },

  


  unmanage: function(aActor) {
    return this.removeActor(aActor);
  },

  


  cleanup: function AP_cleanup() {
    for each (let actor in this._cleanups) {
      actor.disconnect();
    }
    this._cleanups = {};
  }
}














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

  allocID: function DSC_allocID(aPrefix) {
    return this.prefix + (aPrefix || '') + this._nextID++;
  },

  


  addActorPool: function DSC_addActorPool(aActorPool) {
    this._extraPools.push(aActorPool);
  },

  








  removeActorPool: function DSC_removeActorPool(aActorPool, aCleanup) {
    let index = this._extraPools.lastIndexOf(aActorPool);
    if (index > -1) {
      let pool = this._extraPools.splice(index, 1);
      if (aCleanup) {
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
    let errorString = aPrefix + ": " + safeErrorString(aError);
    Cu.reportError(errorString);
    dumpn(errorString);
    return {
      error: "unknownError",
      message: errorString
    };
  },

  

  














  setForwarding: function(aPrefix, aTransport) {
    this._forwardingPrefixes.set(aPrefix, aTransport);
  },

  



  cancelForwarding: function(aPrefix) {
    this._forwardingPrefixes.delete(aPrefix);
  },

  

  





  onPacket: function DSC_onPacket(aPacket) {
    
    
    
    
    
    
    if (this._forwardingPrefixes.size > 0) {
      let colon = aPacket.to.indexOf(':');
      if (colon >= 0) {
        let forwardTo = this._forwardingPrefixes.get(aPacket.to.substring(0, colon));
        if (forwardTo) {
          forwardTo.send(aPacket);
          return;
        }
      }
    }

    let actor = this.getActor(aPacket.to);
    if (!actor) {
      this.transport.send({ from: aPacket.to ? aPacket.to : "root",
                            error: "noSuchActor",
                            message: "No such actor for ID: " + aPacket.to });
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

    var ret = null;
    
    if (actor.requestTypes && actor.requestTypes[aPacket.type]) {
      try {
        this.currentPacket = aPacket;
        ret = actor.requestTypes[aPacket.type].bind(actor)(aPacket, this);
      } catch(e) {
        this.transport.send(this._unknownError(
          "error occurred while processing '" + aPacket.type,
          e));
      } finally {
        delete this.currentPacket;
      }
    } else {
      ret = { error: "unrecognizedPacketType",
              message: ('Actor "' + actor.actorID +
                        '" does not recognize the packet type "' +
                        aPacket.type + '"') };
    }

    if (!ret) {
      
      
      return;
    }

    let pendingResponse = this._actorResponses.get(actor.actorID) || resolve(null);
    let response = pendingResponse.then(() => {
      return ret;
    }).then(aResponse => {
      if (!aResponse.from) {
        aResponse.from = aPacket.to;
      }
      this.transport.send(aResponse);
    }).then(null, (e) => {
      let errorPacket = this._unknownError(
        "error occurred while processing '" + aPacket.type,
        e);
      errorPacket.from = aPacket.to;
      this.transport.send(errorPacket);
    });

    this._actorResponses.set(actor.actorID, response);
  },

  






  onClosed: function DSC_onClosed(aStatus) {
    dumpn("Cleaning up connection.");

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




let L10N = {

  





  getStr: function L10N_getStr(aName) {
    return this.stringBundle.GetStringFromName(aName);
  }
};

XPCOMUtils.defineLazyGetter(L10N, "stringBundle", function() {
  return Services.strings.createBundle(DBG_STRINGS_URI);
});
