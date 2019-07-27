





"use strict";

let { Ci, Cc, CC, Cr } = require("chrome");
let Services = require("Services");
let DevToolsUtils = require("devtools/toolkit/DevToolsUtils");
let { dumpn } = DevToolsUtils;
loader.lazyRequireGetter(this, "DebuggerTransport",
  "devtools/toolkit/transport/transport", true);
loader.lazyRequireGetter(this, "DebuggerServer",
  "devtools/server/main", true);

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

const DBG_STRINGS_URI = "chrome://global/locale/devtools/debugger.properties";









function socketConnect(host, port) {
  let s = socketTransportService.createTransport(null, 0, host, port, null);
  
  
  
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






function SocketListener() {}









SocketListener.defaultAllowConnection = () => {
  let bundle = Services.strings.createBundle(DBG_STRINGS_URI);
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
  if (result === 0) {
    return true;
  }
  if (result === 2) {
    DebuggerServer.closeAllListeners();
    Services.prefs.setBoolPref("devtools.debugger.remote-enabled", false);
  }
  return false;
};

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
    DebuggerServer._removeListener(this);
  },

  



  get port() {
    if (!this._socket) {
      return null;
    }
    return this._socket.port;
  },

  






  allowConnection: SocketListener.defaultAllowConnection,

  

  onSocketAccepted:
  DevToolsUtils.makeInfallible(function(socket, socketTransport) {
    if (Services.prefs.getBoolPref("devtools.debugger.prompt-connection") &&
        !this.allowConnection()) {
      return;
    }
    dumpn("New debugging connection on " +
          socketTransport.host + ":" + socketTransport.port);

    let input = socketTransport.openInputStream(0, 0, 0);
    let output = socketTransport.openOutputStream(0, 0, 0);
    let transport = new DebuggerTransport(input, output);
    DebuggerServer._onConnection(transport);
  }, "SocketListener.onSocketAccepted"),

  onStopListening: function(socket, status) {
    dumpn("onStopListening, status: " + status);
  }

};



exports.DebuggerSocket = {
  createListener() {
    return new SocketListener();
  },
  connect(host, port) {
    return socketConnect(host, port);
  }
};
