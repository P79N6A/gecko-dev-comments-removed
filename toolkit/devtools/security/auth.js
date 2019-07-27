





"use strict";












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
