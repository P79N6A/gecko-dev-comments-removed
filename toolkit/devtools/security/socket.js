





"use strict";

let { Ci, Cc, CC, Cr, Cu } = require("chrome");


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
loader.lazyRequireGetter(this, "Authenticators",
  "devtools/toolkit/security/auth", true);
loader.lazyRequireGetter(this, "AuthenticationResult",
  "devtools/toolkit/security/auth", true);
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

let DebuggerSocket = {};


















DebuggerSocket.connect = Task.async(function*(settings) {
  
  if (!settings.authenticator) {
    settings.authenticator = new (Authenticators.get().Client)();
  }
  let { host, port, encryption, authenticator, cert } = settings;
  let transport = yield _getTransport(settings);
  yield authenticator.authenticate({
    host,
    port,
    encryption,
    cert,
    transport
  });
  return transport;
});
























let _getTransport = Task.async(function*(settings) {
  let { host, port, encryption } = settings;
  let attempt = yield _attemptTransport(settings);
  if (attempt.transport) {
    return attempt.transport; 
  }

  
  
  if (encryption && attempt.certError) {
    _storeCertOverride(attempt.s, host, port);
  } else {
    throw new Error("Connection failed");
  }

  attempt = yield _attemptTransport(settings);
  if (attempt.transport) {
    return attempt.transport; 
  }

  throw new Error("Connection failed even after cert override");
});


























let _attemptTransport = Task.async(function*(settings) {
  let { authenticator } = settings;
  
  
  let { s, input, output } = yield _attemptConnect(settings);

  
  
  let alive, certError;
  try {
    let results = yield _isInputAlive(input);
    alive = results.alive;
    certError = results.certError;
  } catch(e) {
    
    
    input.close();
    output.close();
    throw e;
  }
  dumpv("Server cert accepted? " + !certError);

  
  
  alive = alive && authenticator.validateConnection({
    host: settings.host,
    port: settings.port,
    encryption: settings.encryption,
    cert: settings.cert,
    socket: s
  });

  let transport;
  if (alive) {
    transport = new DebuggerTransport(input, output);
  } else {
    
    input.close();
    output.close();
  }

  return { transport, certError, s };
});















let _attemptConnect = Task.async(function*({ host, port, encryption }) {
  let s;
  if (encryption) {
    s = socketTransportService.createTransport(["ssl"], 1, host, port, null);
  } else {
    s = socketTransportService.createTransport(null, 0, host, port, null);
  }
  
  
  
  s.setTimeout(Ci.nsISocketTransport.TIMEOUT_CONNECT, 2);

  
  
  let clientCert;
  if (encryption) {
    clientCert = yield cert.local.getOrCreate();
  }

  let deferred = promise.defer();
  let input;
  let output;
  
  
  
  
  
  
  
  s.setEventSink({
    onTransportStatus(transport, status) {
      if (status != Ci.nsISocketTransport.STATUS_CONNECTING_TO) {
        return;
      }
      if (encryption) {
        let sslSocketControl =
          transport.securityInfo.QueryInterface(Ci.nsISSLSocketControl);
        sslSocketControl.clientCert = clientCert;
      }
      try {
        input = s.openInputStream(0, 0, 0);
      } catch(e) {
        deferred.reject(e);
      }
      deferred.resolve({ s, input, output });
    }
  }, Services.tm.currentThread);

  
  
  
  try {
    output = s.openOutputStream(0, 0, 0);
  } catch(e) {
    deferred.reject(e);
  }

  deferred.promise.catch(e => {
    if (input) {
      input.close();
    }
    if (output) {
      output.close();
    }
    DevToolsUtils.reportException("_attemptConnect", e);
  });

  return deferred.promise;
});






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

SocketListener.prototype = {

  

  





  portOrPath: null,

  



  discoverable: false,

  


  encryption: false,

  






  authenticator: new (Authenticators.get().Server)(),

  


  _validateOptions: function() {
    if (this.portOrPath === null) {
      throw new Error("Must set a port / path to listen on.");
    }
    if (this.discoverable && !Number(this.portOrPath)) {
      throw new Error("Discovery only supported for TCP sockets.");
    }
    this.authenticator.validateOptions(this);
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
      this._advertise();
    }).catch(e => {
      dumpn("Could not start debugging listener on '" + this.portOrPath +
            "': " + e);
      this.close();
    });
  },

  _advertise: function() {
    if (!this.discoverable || !this.port) {
      return;
    }

    let advertisement = {
      port: this.port,
      encryption: this.encryption,
    };

    this.authenticator.augmentAdvertisement(this, advertisement);

    discovery.addService("devtools", advertisement);
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
    this.authenticator.augmentSocketOptions(this, this._socket);
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

  get host() {
    if (!this._socket) {
      return null;
    }
    if (Services.prefs.getBoolPref("devtools.debugger.force-local")) {
      return "127.0.0.1";
    }
    return "0.0.0.0";
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

  get cert() {
    if (!this._socket || !this._socket.serverCert) {
      return null;
    }
    return {
      sha256: this._socket.serverCert.sha256Fingerprint
    };
  },

  

  onSocketAccepted:
  DevToolsUtils.makeInfallible(function(socket, socketTransport) {
    new ServerSocketConnection(this, socketTransport);
  }, "SocketListener.onSocketAccepted"),

  onStopListening: function(socket, status) {
    dumpn("onStopListening, status: " + status);
  }

};


loader.lazyGetter(this, "HANDSHAKE_TIMEOUT", () => {
  return Services.prefs.getIntPref("devtools.remote.tls-handshake-timeout");
});







function ServerSocketConnection(listener, socketTransport) {
  this._listener = listener;
  this._socketTransport = socketTransport;
  this._handle();
}

ServerSocketConnection.prototype = {

  get authentication() {
    return this._listener.authenticator.mode;
  },

  get host() {
    return this._socketTransport.host;
  },

  get port() {
    return this._socketTransport.port;
  },

  get cert() {
    if (!this._clientCert) {
      return null;
    }
    return {
      sha256: this._clientCert.sha256Fingerprint
    };
  },

  get address() {
    return this.host + ":" + this.port;
  },

  get client() {
    let client = {
      host: this.host,
      port: this.port
    };
    if (this.cert) {
      client.cert = this.cert;
    }
    return client;
  },

  get server() {
    let server = {
      host: this._listener.host,
      port: this._listener.port
    };
    if (this._listener.cert) {
      server.cert = this._listener.cert;
    }
    return server;
  },

  




  _handle() {
    dumpn("Debugging connection starting authentication on " + this.address);
    let self = this;
    Task.spawn(function*() {
      self._listenForTLSHandshake();
      self._createTransport();
      yield self._awaitTLSHandshake();
      yield self._authenticate();
    }).then(() => this.allow()).catch(e => this.deny(e));
  },

  



  _createTransport() {
    let input = this._socketTransport.openInputStream(0, 0, 0);
    let output = this._socketTransport.openOutputStream(0, 0, 0);
    this._transport = new DebuggerTransport(input, output);
    
    
    this._transport.hooks = {
      onClosed: reason => {
        this.deny(reason);
      }
    };
    this._transport.ready();
  },

  



  _setSecurityObserver(observer) {
    if (!this._socketTransport || !this._socketTransport.securityInfo) {
      return;
    }
    let connectionInfo = this._socketTransport.securityInfo
                         .QueryInterface(Ci.nsITLSServerConnectionInfo);
    connectionInfo.setSecurityObserver(observer);
  },

  




  _listenForTLSHandshake() {
    this._handshakeDeferred = promise.defer();
    if (!this._listener.encryption) {
      this._handshakeDeferred.resolve();
      return;
    }
    this._setSecurityObserver(this);
    this._handshakeTimeout = setTimeout(this._onHandshakeTimeout.bind(this),
                                        HANDSHAKE_TIMEOUT);
  },

  _awaitTLSHandshake() {
    return this._handshakeDeferred.promise;
  },

  _onHandshakeTimeout() {
    dumpv("Client failed to complete TLS handshake");
    this._handshakeDeferred.reject(Cr.NS_ERROR_NET_TIMEOUT);
  },

  
  onHandshakeDone(socket, clientStatus) {
    clearTimeout(this._handshakeTimeout);
    this._setSecurityObserver(null);
    dumpv("TLS version:    " + clientStatus.tlsVersionUsed.toString(16));
    dumpv("TLS cipher:     " + clientStatus.cipherName);
    dumpv("TLS key length: " + clientStatus.keyLength);
    dumpv("TLS MAC length: " + clientStatus.macLength);
    this._clientCert = clientStatus.peerCert;
    







    if (clientStatus.tlsVersionUsed != Ci.nsITLSClientStatus.TLS_VERSION_1_2) {
      this._handshakeDeferred.reject(Cr.NS_ERROR_CONNECTION_REFUSED);
      return;
    }

    this._handshakeDeferred.resolve();
  },

  _authenticate: Task.async(function*() {
    let result = yield this._listener.authenticator.authenticate({
      client: this.client,
      server: this.server,
      transport: this._transport
    });
    switch (result) {
      case AuthenticationResult.DISABLE_ALL:
        DebuggerServer.closeAllListeners();
        Services.prefs.setBoolPref("devtools.debugger.remote-enabled", false);
        return promise.reject(Cr.NS_ERROR_CONNECTION_REFUSED);
      case AuthenticationResult.DENY:
        return promise.reject(Cr.NS_ERROR_CONNECTION_REFUSED);
      case AuthenticationResult.ALLOW:
      case AuthenticationResult.ALLOW_PERSIST:
        return promise.resolve();
      default:
        return promise.reject(Cr.NS_ERROR_CONNECTION_REFUSED);
    }
  }),

  deny(result) {
    if (this._destroyed) {
      return;
    }
    let errorName = result;
    for (let name in Cr) {
      if (Cr[name] === result) {
        errorName = name;
        break;
      }
    }
    dumpn("Debugging connection denied on " + this.address +
          " (" + errorName + ")");
    this._transport.hooks = null;
    this._transport.close(result);
    this._socketTransport.close(result);
    this.destroy();
  },

  allow() {
    if (this._destroyed) {
      return;
    }
    dumpn("Debugging connection allowed on " + this.address);
    DebuggerServer._onConnection(this._transport);
    this.destroy();
  },

  destroy() {
    this._destroyed = true;
    clearTimeout(this._handshakeTimeout);
    this._setSecurityObserver(null);
    this._listener = null;
    this._socketTransport = null;
    this._transport = null;
    this._clientCert = null;
  }

};

DebuggerSocket.createListener = function() {
  return new SocketListener();
};

exports.DebuggerSocket = DebuggerSocket;
