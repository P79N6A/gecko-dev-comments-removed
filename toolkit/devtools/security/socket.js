





"use strict";

let { Ci, Cc, CC, Cr } = require("chrome");


Cc["@mozilla.org/psm;1"].getService(Ci.nsISupports);

let Services = require("Services");
let promise = require("promise");
let DevToolsUtils = require("devtools/toolkit/DevToolsUtils");
let { dumpn, dumpv } = DevToolsUtils;
loader.lazyRequireGetter(this, "DebuggerTransport",
  "devtools/toolkit/transport/transport", true);
loader.lazyRequireGetter(this, "DebuggerServer",
  "devtools/server/main", true);
loader.lazyRequireGetter(this, "discovery",
  "devtools/toolkit/discovery/discovery");
loader.lazyRequireGetter(this, "cert",
  "devtools/toolkit/security/cert");
loader.lazyRequireGetter(this, "setTimeout", "Timer", true);
loader.lazyRequireGetter(this, "clearTimeout", "Timer", true);

DevToolsUtils.defineLazyGetter(this, "nsFile", () => {
  return CC("@mozilla.org/file/local;1", "nsIFile", "initWithPath");
});

DevToolsUtils.defineLazyGetter(this, "socketTransportService", () => {
  return Cc["@mozilla.org/network/socket-transport-service;1"]
         .getService(Ci.nsISocketTransportService);
});

DevToolsUtils.defineLazyGetter(this, "certOverrideService", () => {
  return Cc["@mozilla.org/security/certoverride;1"]
         .getService(Ci.nsICertOverrideService);
});

DevToolsUtils.defineLazyGetter(this, "nssErrorsService", () => {
  return Cc["@mozilla.org/nss_errors_service;1"]
         .getService(Ci.nsINSSErrorsService);
});

DevToolsUtils.defineLazyModuleGetter(this, "Task",
  "resource://gre/modules/Task.jsm");

const DBG_STRINGS_URI = "chrome://global/locale/devtools/debugger.properties";

let DebuggerSocket = {};













DebuggerSocket.connect = Task.async(function*({ host, port, encryption }) {
  let attempt = yield _attemptTransport({ host, port, encryption });
  if (attempt.transport) {
    return attempt.transport; 
  }

  
  
  if (encryption && attempt.certError) {
    _storeCertOverride(attempt.s, host, port);
  } else {
    throw new Error("Connection failed");
  }

  attempt = yield _attemptTransport({ host, port, encryption });
  if (attempt.transport) {
    return attempt.transport; 
  }

  throw new Error("Connection failed even after cert override");
});












let _attemptTransport = Task.async(function*({ host, port, encryption }){
  
  
  let { s, input, output } = _attemptConnect({ host, port, encryption });

  
  
  let { alive, certError } = yield _isInputAlive(input);
  dumpv("Server cert accepted? " + !certError);

  let transport;
  if (alive) {
    transport = new DebuggerTransport(input, output);
  } else {
    
    input.close();
    output.close();
  }

  return { transport, certError, s };
});















function _attemptConnect({ host, port, encryption }) {
  let s;
  if (encryption) {
    s = socketTransportService.createTransport(["ssl"], 1, host, port, null);
  } else {
    s = socketTransportService.createTransport(null, 0, host, port, null);
  }
  
  
  
  s.setTimeout(Ci.nsISocketTransport.TIMEOUT_CONNECT, 2);

  
  
  
  let input;
  let output;
  try {
    input = s.openInputStream(0, 0, 0);
    output = s.openOutputStream(0, 0, 0);
  } catch(e) {
    DevToolsUtils.reportException("_attemptConnect", e);
    throw e;
  }

  return { s, input, output };
}






function _isInputAlive(input) {
  let deferred = promise.defer();
  input.asyncWait({
    onInputStreamReady(stream) {
      try {
        stream.available();
        deferred.resolve({ alive: true });
      } catch (e) {
        try {
          
          let errorClass = nssErrorsService.getErrorClass(e.result);
          if (errorClass === Ci.nsINSSErrorsService.ERROR_CLASS_BAD_CERT) {
            deferred.resolve({ certError: true });
          } else {
            deferred.reject(e);
          }
        } catch (nssErr) {
          deferred.reject(e);
        }
      }
    }
  }, 0, 0, Services.tm.currentThread);
  return deferred.promise;
}






function _storeCertOverride(s, host, port) {
  let cert = s.securityInfo.QueryInterface(Ci.nsISSLStatusProvider)
              .SSLStatus.serverCert;
  let overrideBits = Ci.nsICertOverrideService.ERROR_UNTRUSTED |
                     Ci.nsICertOverrideService.ERROR_MISMATCH;
  certOverrideService.rememberValidityOverride(host, port, cert, overrideBits,
                                               true );
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

  

  





  portOrPath: null,

  






  allowConnection: SocketListener.defaultAllowConnection,

  



  discoverable: false,

  


  encryption: false,

  


  _validateOptions: function() {
    if (this.portOrPath === null) {
      throw new Error("Must set a port / path to listen on.");
    }
    if (this.discoverable && !Number(this.portOrPath)) {
      throw new Error("Discovery only supported for TCP sockets.");
    }
  },

  


  open: function() {
    this._validateOptions();
    DebuggerServer._addListener(this);

    let flags = Ci.nsIServerSocket.KeepWhenOffline;
    
    if (Services.prefs.getBoolPref("devtools.debugger.force-local")) {
      flags |= Ci.nsIServerSocket.LoopbackOnly;
    }

    let self = this;
    return Task.spawn(function*() {
      let backlog = 4;
      self._socket = self._createSocketInstance();
      if (self.isPortBased) {
        let port = Number(self.portOrPath);
        self._socket.initSpecialConnection(port, flags, backlog);
      } else {
        let file = nsFile(self.portOrPath);
        if (file.exists()) {
          file.remove(false);
        }
        self._socket.initWithFilename(file, parseInt("666", 8), backlog);
      }
      yield self._setAdditionalSocketOptions();
      self._socket.asyncListen(self);
      dumpn("Socket listening on: " + (self.port || self.portOrPath));
    }).then(() => {
      if (this.discoverable && this.port) {
        discovery.addService("devtools", {
          port: this.port,
          encryption: this.encryption
        });
      }
    }).catch(e => {
      dumpn("Could not start debugging listener on '" + this.portOrPath +
            "': " + e);
      this.close();
    });
  },

  _createSocketInstance: function() {
    if (this.encryption) {
      return Cc["@mozilla.org/network/tls-server-socket;1"]
             .createInstance(Ci.nsITLSServerSocket);
    }
    return Cc["@mozilla.org/network/server-socket;1"]
           .createInstance(Ci.nsIServerSocket);
  },

  _setAdditionalSocketOptions: Task.async(function*() {
    if (this.encryption) {
      this._socket.serverCert = yield cert.local.getOrCreate();
      this._socket.setSessionCache(false);
      this._socket.setSessionTickets(false);
      let requestCert = Ci.nsITLSServerSocket.REQUEST_NEVER;
      this._socket.setRequestClientCertificate(requestCert);
    }
  }),

  



  close: function() {
    if (this.discoverable && this.port) {
      discovery.removeService("devtools");
    }
    if (this._socket) {
      this._socket.close();
      this._socket = null;
    }
    DebuggerServer._removeListener(this);
  },

  


  get isPortBased() {
    return !!Number(this.portOrPath);
  },

  



  get port() {
    if (!this.isPortBased || !this._socket) {
      return null;
    }
    return this._socket.port;
  },

  

  onSocketAccepted:
  DevToolsUtils.makeInfallible(function(socket, socketTransport) {
    if (this.encryption) {
      new SecurityObserver(socketTransport);
    }
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


loader.lazyGetter(this, "HANDSHAKE_TIMEOUT", () => {
  return Services.prefs.getIntPref("devtools.remote.tls-handshake-timeout");
});

function SecurityObserver(socketTransport) {
  this.socketTransport = socketTransport;
  let connectionInfo = socketTransport.securityInfo
                       .QueryInterface(Ci.nsITLSServerConnectionInfo);
  connectionInfo.setSecurityObserver(this);
  this._handshakeTimeout = setTimeout(this._onHandshakeTimeout.bind(this),
                                      HANDSHAKE_TIMEOUT);
}

SecurityObserver.prototype = {

  _onHandshakeTimeout() {
    dumpv("Client failed to complete handshake");
    this.destroy(Cr.NS_ERROR_NET_TIMEOUT);
  },

  
  onHandshakeDone(socket, clientStatus) {
    clearTimeout(this._handshakeTimeout);
    dumpv("TLS version:    " + clientStatus.tlsVersionUsed.toString(16));
    dumpv("TLS cipher:     " + clientStatus.cipherName);
    dumpv("TLS key length: " + clientStatus.keyLength);
    dumpv("TLS MAC length: " + clientStatus.macLength);
    










    if (clientStatus.tlsVersionUsed != Ci.nsITLSClientStatus.TLS_VERSION_1_2) {
      this.destroy(Cr.NS_ERROR_CONNECTION_REFUSED);
    }
  },

  destroy(result) {
    clearTimeout(this._handshakeTimeout);
    let connectionInfo = this.socketTransport.securityInfo
                         .QueryInterface(Ci.nsITLSServerConnectionInfo);
    connectionInfo.setSecurityObserver(null);
    this.socketTransport.close(result);
    this.socketTransport = null;
  }

};

DebuggerSocket.createListener = function() {
  return new SocketListener();
};

exports.DebuggerSocket = DebuggerSocket;
