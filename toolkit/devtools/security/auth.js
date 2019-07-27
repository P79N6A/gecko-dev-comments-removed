





"use strict";

let Services = require("Services");
loader.lazyRequireGetter(this, "prompt",
  "devtools/toolkit/security/prompt");












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

  








  augmentAdvertisement(listener, advertisement) {
    advertisement.authentication = Prompt.mode;
  },

  

















  authenticate(session) {
    if (!Services.prefs.getBoolPref("devtools.debugger.prompt-connection")) {
      return true;
    }
    session.authentication = this.mode;
    return this.allowConnection(session);
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

  








  augmentAdvertisement(listener, advertisement) {
    advertisement.authentication = OOBCert.mode;
    
    
    
    advertisement.cert = {
      sha256: listener._socket.serverCert.sha256Fingerprint
    };
  },

  























  authenticate(session) {
    session.authentication = this.mode;
    return this.allowConnection(session);
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
