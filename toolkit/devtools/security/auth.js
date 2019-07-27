





"use strict";

let { Ci, Cc } = require("chrome");
let Services = require("Services");
let promise = require("promise");
let DevToolsUtils = require("devtools/toolkit/DevToolsUtils");
let { dumpn, dumpv } = DevToolsUtils;
loader.lazyRequireGetter(this, "prompt",
  "devtools/toolkit/security/prompt");
loader.lazyRequireGetter(this, "cert",
  "devtools/toolkit/security/cert");
DevToolsUtils.defineLazyModuleGetter(this, "Task",
  "resource://gre/modules/Task.jsm");






function createEnum(obj) {
  for (let key in obj) {
    obj[key] = key;
  }
  return obj;
}







let AuthenticationResult = exports.AuthenticationResult = createEnum({

  


  DISABLE_ALL: null,

  


  DENY: null,

  


  PENDING: null,

  


  ALLOW: null,

  




  ALLOW_PERSIST: null

});












let Authenticators = {};







let Prompt = Authenticators.Prompt = {};

Prompt.mode = "PROMPT";

Prompt.Client = function() {};
Prompt.Client.prototype = {

  mode: Prompt.mode,

  
















  validateConnection() {
    return true;
  },

  















  authenticate() {},

};

Prompt.Server = function() {};
Prompt.Server.prototype = {

  mode: Prompt.mode,

  






  validateOptions() {},

  







  augmentSocketOptions() {},

  








  augmentAdvertisement(listener, advertisement) {
    advertisement.authentication = Prompt.mode;
  },

  



















  authenticate({ client, server }) {
    if (!Services.prefs.getBoolPref("devtools.debugger.prompt-connection")) {
      return AuthenticationResult.ALLOW;
    }
    return this.allowConnection({
      authentication: this.mode,
      client,
      server
    });
  },

  























  allowConnection: prompt.Server.defaultAllowConnection,

};



















let OOBCert = Authenticators.OOBCert = {};

OOBCert.mode = "OOB_CERT";

OOBCert.Client = function() {};
OOBCert.Client.prototype = {

  mode: OOBCert.mode,

  
















  validateConnection({ cert, socket }) {
    
    
    
    dumpv("Validate server cert hash");
    let serverCert = socket.securityInfo.QueryInterface(Ci.nsISSLStatusProvider)
                           .SSLStatus.serverCert;
    let advertisedCert = cert;
    if (serverCert.sha256Fingerprint != advertisedCert.sha256) {
      dumpn("Server cert hash doesn't match advertisement");
      return false;
    }
    return true;
  },

  

















  authenticate({ host, port, cert, transport }) {
    let deferred = promise.defer();
    let oobData;

    let activeSendDialog;
    let closeDialog = () => {
      
      
      if (activeSendDialog && activeSendDialog.close) {
        activeSendDialog.close();
        activeSendDialog = null;
      }
    };

    transport.hooks = {
      onPacket: Task.async(function*(packet) {
        closeDialog();
        let { authResult } = packet;
        switch (authResult) {
          case AuthenticationResult.PENDING:
            
            
            oobData = yield this._createOOB();
            activeSendDialog = this.sendOOB({
              host,
              port,
              cert,
              authResult,
              oob: oobData
            });
            break;
          case AuthenticationResult.ALLOW:
          case AuthenticationResult.ALLOW_PERSIST:
            
            
            if (packet.k != oobData.k) {
              transport.close(new Error("Auth secret mismatch"));
              return;
            }
            
            
            transport.hooks = null;
            deferred.resolve(transport);
            break;
          default:
            transport.close(new Error("Invalid auth result: " + authResult));
            return;
        }
      }.bind(this)),
      onClosed(reason) {
        closeDialog();
        
        transport.hooks = null;
        deferred.reject(reason);
      }
    };
    transport.ready();
    return deferred.promise;
  },

  



  _createOOB: Task.async(function*() {
    let clientCert = yield cert.local.getOrCreate();
    return {
      sha256: clientCert.sha256Fingerprint,
      k: this._createRandom()
    };
  }),

  _createRandom() {
    const length = 16; 
    let rng = Cc["@mozilla.org/security/random-generator;1"]
              .createInstance(Ci.nsIRandomGenerator);
    let bytes = rng.generateRandomBytes(length);
    return [for (byte of bytes) byte.toString(16)].join("");
  },

  

















  sendOOB: prompt.Client.defaultSendOOB,

};

OOBCert.Server = function() {};
OOBCert.Server.prototype = {

  mode: OOBCert.mode,

  






  validateOptions(listener) {
    if (!listener.encryption) {
      throw new Error(OOBCert.mode + " authentication requires encryption.");
    }
  },

  







  augmentSocketOptions(listener, socket) {
    let requestCert = Ci.nsITLSServerSocket.REQUIRE_ALWAYS;
    socket.setRequestClientCertificate(requestCert);
  },

  








  augmentAdvertisement(listener, advertisement) {
    advertisement.authentication = OOBCert.mode;
    
    
    
    advertisement.cert = listener.cert;
  },

  

























  authenticate: Task.async(function*({ client, server, transport }) {
    
    
    
    
    
    
    transport.send({
      authResult: AuthenticationResult.PENDING
    });

    
    
    
    let result = yield this.allowConnection({
      authentication: this.mode,
      client,
      server
    });

    switch (result) {
      case AuthenticationResult.ALLOW_PERSIST:
        
      case AuthenticationResult.ALLOW:
        break; 
      default:
        return result; 
    }

    
    let oob = yield this.receiveOOB();
    if (!oob) {
      dumpn("Invalid OOB data received");
      return AuthenticationResult.DENY;
    }

    let { sha256, k } = oob;
    
    
    
    if (!sha256 || !k) {
      dumpn("Invalid OOB data received");
      return AuthenticationResult.DENY;
    }

    
    
    
    if (client.cert.sha256 != sha256) {
      dumpn("Client cert hash doesn't match OOB data");
      return AuthenticationResult.DENY;
    }

    
    
    transport.send({ authResult: result, k });

    
    

    
    
    return result;
  }),

  





























  allowConnection: prompt.Server.defaultAllowConnection,

  








  receiveOOB: prompt.Server.defaultReceiveOOB,

};

exports.Authenticators = {
  get(mode) {
    if (!mode) {
      mode = Prompt.mode;
    }
    for (let key in Authenticators) {
      let auth = Authenticators[key];
      if (auth.mode === mode) {
        return auth;
      }
    }
    throw new Error("Unknown authenticator mode: " + mode);
  }
};
