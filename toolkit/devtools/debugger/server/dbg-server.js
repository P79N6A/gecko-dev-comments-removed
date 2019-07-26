





"use strict";





const Ci = Components.interfaces;
const Cc = Components.classes;
const CC = Components.Constructor;
const Cu = Components.utils;
const Cr = Components.results;
const DBG_STRINGS_URI = "chrome://global/locale/devtools/debugger.properties";

Cu.import("resource://gre/modules/Services.jsm");
Cu.import("resource://gre/modules/XPCOMUtils.jsm");
let wantLogging = Services.prefs.getBoolPref("devtools.debugger.log");

Cu.import("resource://gre/modules/jsdebugger.jsm");
addDebuggerToGlobal(this);

Cu.import("resource://gre/modules/commonjs/sdk/core/promise.js");
const { defer, resolve, reject } = Promise;

function dumpn(str) {
  if (wantLogging) {
    dump("DBG-SERVER: " + str + "\n");
  }
}

function dbg_assert(cond, e) {
  if (!cond) {
    return e;
  }
}


function safeErrorString(aError) {
  try {
    var s = aError.toString();
    if (typeof s === "string")
      return s;
  } catch (ee) { }

  return "<failed trying to find error description>";
}

loadSubScript.call(this, "chrome://global/content/devtools/dbg-transport.js");


const ServerSocket = CC("@mozilla.org/network/server-socket;1",
                        "nsIServerSocket",
                        "init");




var DebuggerServer = {
  _listener: null,
  _transportInitialized: false,
  xpcInspector: null,
  
  _socketConnections: 0,
  
  globalActorFactories: null,
  
  tabActorFactories: null,

  LONG_STRING_LENGTH: 10000,
  LONG_STRING_INITIAL_LENGTH: 1000,

  



  _allowConnection: null,

  






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
    this.addActors("chrome://global/content/devtools/dbg-script-actors.js");

    this.globalActorFactories = {};
    this.tabActorFactories = {};
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

  get initialized() { return !!this.globalActorFactories; },

  






  destroy: function DS_destroy() {
    if (Object.keys(this._connections).length == 0) {
      this.closeListener();
      delete this.globalActorFactories;
      delete this.tabActorFactories;
      delete this._allowConnection;
      this._transportInitialized = false;
      dumpn("Debugger server is shut down.");
    }
  },

  







  addActors: function DS_addActors(aURL) {
    loadSubScript.call(this, aURL);
  },

  


  addBrowserActors: function DS_addBrowserActors() {
    this.addActors("chrome://global/content/devtools/dbg-browser-actors.js");
#ifndef MOZ_WIDGET_GONK
    this.addActors("chrome://global/content/devtools/dbg-webconsole-actors.js");
    this.addTabActor(this.WebConsoleActor, "consoleActor");
    this.addGlobalActor(this.WebConsoleActor, "consoleActor");
#endif
    if ("nsIProfiler" in Ci)
      this.addActors("chrome://global/content/devtools/dbg-profiler-actors.js");
  },

  





  openListener: function DS_openListener(aPort) {
    if (!Services.prefs.getBoolPref("devtools.debugger.remote-enabled")) {
      return false;
    }
    this._checkInit();

    
    if (this._listener) {
      return true;
    }

    let localOnly = false;
    
    if (Services.prefs.getBoolPref("devtools.debugger.force-local")) {
      localOnly = true;
    }

    try {
      let socket = new ServerSocket(aPort, localOnly, 4);
      socket.asyncListen(this);
      this._listener = socket;
    } catch (e) {
      dumpn("Could not start debugging listener on port " + aPort + ": " + e);
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

  







  connectPipe: function DS_connectPipe() {
    this._checkInit();

    let serverTransport = new LocalDebuggerTransport;
    let clientTransport = new LocalDebuggerTransport(serverTransport);
    serverTransport.other = clientTransport;
    this._onConnection(serverTransport);

    return clientTransport;
  },


  

  onSocketAccepted: function DS_onSocketAccepted(aSocket, aTransport) {
    if (!this._allowConnection()) {
      return;
    }
    dumpn("New debugging connection on " + aTransport.host + ":" + aTransport.port);

    try {
      let input = aTransport.openInputStream(0, 0, 0);
      let output = aTransport.openOutputStream(0, 0, 0);
      let transport = new DebuggerTransport(input, output);
      DebuggerServer._onConnection(transport);
    } catch (e) {
      dumpn("Couldn't initialize connection: " + e + " - " + e.stack);
    }
  },

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

  



  _onConnection: function DS_onConnection(aTransport) {
    let connID = "conn" + this._nextConnID++ + '.';
    let conn = new DebuggerServerConnection(connID, aTransport);
    this._connections[connID] = conn;

    
    conn.rootActor = this.createRootActor(conn);
    conn.addActor(conn.rootActor);
    aTransport.send(conn.rootActor.sayHello());
    aTransport.ready();
  },

  


  _connectionClosed: function DS_connectionClosed(aConnection) {
    delete this._connections[aConnection.prefix];
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









function ActorPool(aConnection)
{
  this.conn = aConnection;
  this._cleanups = {};
  this._actors = {};
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
}

DebuggerServerConnection.prototype = {
  _prefix: null,
  get prefix() { return this._prefix },

  _transport: null,
  get transport() { return this._transport },

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

  






  getActor: function DSC_getActor(aActorID) {
    if (this._actorPool.has(aActorID)) {
      return this._actorPool.get(aActorID);
    }

    for each (let pool in this._extraPools) {
      if (pool.has(aActorID)) {
        return pool.get(aActorID);
      }
    }

    if (aActorID === "root") {
      return this.rootActor;
    }

    return null;
  },

  

  





  onPacket: function DSC_onPacket(aPacket) {
    let actor = this.getActor(aPacket.to);
    if (!actor) {
      this.transport.send({ from: aPacket.to ? aPacket.to : "root",
                            error: "noSuchActor" });
      return;
    }

    
    if (typeof actor == "function") {
      let instance;
      try {
        instance = new actor();
      } catch (e) {
        Cu.reportError(e);
        this.transport.send({
          error: "unknownError",
          message: ("error occurred while creating actor '" + actor.name +
                    "': " + safeErrorString(e))
        });
      }
      instance.parentID = actor.parentID;
      
      
      
      instance.actorID = actor.actorID;
      actor.registeredPool.addActor(instance);
      actor = instance;
    }

    var ret = null;

    
    if (actor.requestTypes && actor.requestTypes[aPacket.type]) {
      try {
        ret = actor.requestTypes[aPacket.type].bind(actor)(aPacket);
      } catch(e) {
        Cu.reportError(e);
        ret = { error: "unknownError",
                message: ("error occurred while processing '" + aPacket.type +
                          "' request: " + safeErrorString(e)) };
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

    resolve(ret).then(function(returnPacket) {
      if (!returnPacket.from) {
        returnPacket.from = aPacket.to;
      }
      this.transport.send(returnPacket);
    }.bind(this));
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
