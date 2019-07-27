





"use strict";

let { Ci } = require("chrome");
let Services = require("Services");
loader.lazyRequireGetter(this, "prompt",
  "devtools/toolkit/security/prompt");






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

  

























  authenticate({ client, server, transport }) {
    
    
    
    
    
    
    transport.send({
      authResult: AuthenticationResult.PENDING
    });

    
    
    
    return this.allowConnection({
      authentication: this.mode,
      client,
      server
    });
  },

  




























  allowConnection: prompt.Server.defaultAllowConnection,

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
