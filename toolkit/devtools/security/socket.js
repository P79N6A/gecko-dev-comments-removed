





"use strict";

let { Ci, Cc, CC, Cr } = require("chrome");
let Services = require("Services");
let DevToolsUtils = require("devtools/toolkit/DevToolsUtils");
let { dumpn } = DevToolsUtils;
let { DebuggerTransport } = require("devtools/toolkit/transport/transport");

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

DevToolsUtils.defineLazyGetter(this, "nsFile", () => {
  return CC("@mozilla.org/file/local;1", "nsIFile", "initWithPath");
});

DevToolsUtils.defineLazyGetter(this, "socketTransportService", () => {
  return Cc["@mozilla.org/network/socket-transport-service;1"]
         .getService(Ci.nsISocketTransportService);
});









function socketConnect(aHost, aPort)
{
  let s = socketTransportService.createTransport(null, 0, aHost, aPort, null);
  
  
  
  s.setTimeout(Ci.nsISocketTransport.TIMEOUT_CONNECT, 2);

  
  
  
  let transport;
  try {
    transport = new DebuggerTransport(s.openInputStream(0, 0, 0),
                                      s.openOutputStream(0, 0, 0));
  } catch(e) {
    DevToolsUtils.reportException("socketConnect", e);
    throw e;
  }
  return transport;
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



exports.DebuggerSocket = {
  createListener(server) {
    return new SocketListener(server);
  },
  connect(host, port) {
    return socketConnect(host, port);
  }
};
