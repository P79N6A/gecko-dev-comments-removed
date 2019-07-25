



































const EXPORTED_SYMBOLS = ['Auth', 'BrokenBasicAuthenticator',
                          'BasicAuthenticator', 'NoOpAuthenticator'];

const Cc = Components.classes;
const Ci = Components.interfaces;
const Cr = Components.results;
const Cu = Components.utils;

Cu.import("resource://services-sync/util.js");

Utils.lazy(this, 'Auth', AuthMgr);




function NoOpAuthenticator() {}
NoOpAuthenticator.prototype = {
  onRequest: function NoOpAuth_onRequest(headers) {
    return headers;
  }
};



function BrokenBasicAuthenticator(identity) {
  this._id = identity;
}
BrokenBasicAuthenticator.prototype = {
  onRequest: function BasicAuth_onRequest(headers) {
    headers['Authorization'] = 'Basic ' +
      btoa(this._id.username + ':' + this._id.password);
    return headers;
  }
};

function BasicAuthenticator(identity) {
  this._id = identity;
}
BasicAuthenticator.prototype = {
  onRequest: function onRequest(headers) {
    headers['Authorization'] = 'Basic ' +
      btoa(this._id.username + ':' + this._id.passwordUTF8);
    return headers;
  }
};

function AuthMgr() {
  this._authenticators = {};
  this.defaultAuthenticator = new NoOpAuthenticator();
}
AuthMgr.prototype = {
  defaultAuthenticator: null,

  registerAuthenticator: function AuthMgr_register(match, authenticator) {
    this._authenticators[match] = authenticator;
  },

  lookupAuthenticator: function AuthMgr_lookup(uri) {
    for (let match in this._authenticators) {
      if (uri.match(match))
        return this._authenticators[match];
    }
    return this.defaultAuthenticator;
  }
};
