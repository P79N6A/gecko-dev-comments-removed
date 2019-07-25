





"use strict";





const Ci = Components.interfaces;
const Cc = Components.classes;
const CC = Components.Constructor;
const Cu = Components.utils;
const Cr = Components.results;

Cu.import("resource://gre/modules/Services.jsm");
let wantLogging = Services.prefs.getBoolPref("devtools.debugger.log");

Cu.import("resource://gre/modules/jsdebugger.jsm");
addDebuggerToGlobal(this);

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
  _allowConnection: null,

  






  init: function DH_init(aAllowConnectionCallback) {
    if (this.initialized) {
      return;
    }

    this.xpcInspector = Cc["@mozilla.org/jsinspector;1"].getService(Ci.nsIJSInspector);
    this.initTransport(aAllowConnectionCallback);
    this.addActors("chrome://global/content/devtools/dbg-script-actors.js");
  },

  







  initTransport: function DH_initTransport(aAllowConnectionCallback) {
    if (this._transportInitialized) {
      return;
    }

    this._connections = {};
    this._nextConnID = 0;
    this._transportInitialized = true;
    this._allowConnection = aAllowConnectionCallback;
  },

  get initialized() { return !!this.xpcInspector; },

  







  addActors: function DH_addActors(aURL) {
    loadSubScript.call(this, aURL);
  },

  


  addBrowserActors: function DH_addBrowserActors() {
    this.addActors("chrome://global/content/devtools/dbg-browser-actors.js");
  },

  





  openListener: function DH_openListener(aPort) {
    if (!Services.prefs.getBoolPref("devtools.debugger.remote-enabled")) {
      return false;
    }
    this._checkInit();

    if (this._listener) {
      throw "Debugging listener already open.";
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

    return true;
  },

  


  closeListener: function DH_closeListener() {
    this._checkInit();

    if (!this._listener) {
      return false;
    }

    this._listener.close();
    this._listener = null;

    return true;
  },

  






  connectPipe: function DH_connectPipe() {
    this._checkInit();

    let toServer = Cc["@mozilla.org/pipe;1"].createInstance(Ci.nsIPipe);
    toServer.init(true, true, 0, 0, null);
    let toClient = Cc["@mozilla.org/pipe;1"].createInstance(Ci.nsIPipe);
    toClient.init(true, true, 0, 0, null);

    let serverTransport = new DebuggerTransport(toServer.inputStream,
                                                toClient.outputStream);
    this._onConnection(serverTransport);

    return new DebuggerTransport(toClient.inputStream, toServer.outputStream);
  },


  

  onSocketAccepted: function DH_onSocketAccepted(aSocket, aTransport) {
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

  onStopListening: function DH_onStopListening() { },

  


  _checkInit: function DH_checkInit() {
    if (!this._transportInitialized) {
      throw "DebuggerServer has not been initialized.";
    }

    if (!this.createRootActor) {
      throw "Use DebuggerServer.addActors() to add a root actor implementation.";
    }
  },

  



  _onConnection: function DH_onConnection(aTransport) {
    if (!this._allowConnection()) {
      return;
    }
    let connID = "conn" + this._nextConnID++ + '.';
    let conn = new DebuggerServerConnection(connID, aTransport);
    this._connections[connID] = conn;

    
    conn.rootActor = this.createRootActor(conn);
    conn.addActor(conn.rootActor);
    aTransport.send(conn.rootActor.sayHello());
    aTransport.ready();
  },

  


  _connectionClosed: function DH_connectionClosed(aConnection) {
    delete this._connections[aConnection.prefix];
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
      aActor.actorID = this.conn.allocID(aActor.actorPrefix || undefined);
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

  


  removeActor: function AP_remove(aActorID) {
    delete this._actors[aActorID];
    delete this._cleanups[aActorID];
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

  


  removeActorPool: function DSC_removeActorPool(aActorPool) {
    let index = this._extraPools.splice(this._extraPools.lastIndexOf(aActorPool), 1);
  },

  


  addActor: function DSC_addActor(aActor) {
    this._actorPool.addActor(aActor);
  },

  


  removeActor: function DSC_removeActor(aActor) {
    this._actorPool.removeActor(aActor);
  },

  


  addCleanup: function DSC_addCleanup(aCleanup) {
    this._actorPool.addCleanup(aCleanup);
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

    if (!ret.from) {
      ret.from = aPacket.to;
    }

    this.transport.send(ret);
  },

  






  onClosed: function DSC_onClosed(aStatus) {
    dumpn("Cleaning up connection.");

    this._actorPool.cleanup();
    this._actorPool = null;
    this._extraPools.map(function(p) { p.cleanup(); });
    this._extraPools = null;

    DebuggerServer._connectionClosed(this);
  }
};
