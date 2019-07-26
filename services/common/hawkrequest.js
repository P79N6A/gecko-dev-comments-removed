



"use strict";

const {classes: Cc, interfaces: Ci, utils: Cu, results: Cr} = Components;

this.EXPORTED_SYMBOLS = [
  "HAWKAuthenticatedRESTRequest",
  "deriveHawkCredentials"
];

Cu.import("resource://gre/modules/Preferences.jsm");
Cu.import("resource://gre/modules/Services.jsm");
Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://gre/modules/Log.jsm");
Cu.import("resource://services-common/rest.js");
Cu.import("resource://services-common/utils.js");
Cu.import("resource://gre/modules/Credentials.jsm");

XPCOMUtils.defineLazyModuleGetter(this, "CryptoUtils",
                                  "resource://services-crypto/utils.js");

const Prefs = new Preferences("services.common.rest.");



























this.HAWKAuthenticatedRESTRequest =
 function HawkAuthenticatedRESTRequest(uri, credentials, extra={}) {
  RESTRequest.call(this, uri);

  this.credentials = credentials;
  this.now = extra.now || Date.now();
  this.localtimeOffsetMsec = extra.localtimeOffsetMsec || 0;
  this._log.trace("local time, offset: " + this.now + ", " + (this.localtimeOffsetMsec));

  
  this._intl = getIntl();
};
HAWKAuthenticatedRESTRequest.prototype = {
  __proto__: RESTRequest.prototype,

  dispatch: function dispatch(method, data, onComplete, onProgress) {
    let contentType = "text/plain";
    if (method == "POST" || method == "PUT") {
      contentType = "application/json";
    }
    if (this.credentials) {
      let options = {
        now: this.now,
        localtimeOffsetMsec: this.localtimeOffsetMsec,
        credentials: this.credentials,
        payload: data && JSON.stringify(data) || "",
        contentType: contentType,
      };
      let header = CryptoUtils.computeHAWK(this.uri, method, options);
      this.setHeader("Authorization", header.field);
      this._log.trace("hawk auth header: " + header.field);
    }

    this.setHeader("Content-Type", contentType);

    this.setHeader("Accept-Language", this._intl.accept_languages);

    return RESTRequest.prototype.dispatch.call(
      this, method, data, onComplete, onProgress
    );
  }
};

























this.deriveHawkCredentials = function deriveHawkCredentials(tokenHex,
                                                            context,
                                                            size = 96,
                                                            hexKey = false) {
  let token = CommonUtils.hexToBytes(tokenHex);
  let out = CryptoUtils.hkdf(token, undefined, Credentials.keyWord(context), size);

  let result = {
    algorithm: "sha256",
    key: hexKey ? CommonUtils.bytesAsHex(out.slice(32, 64)) : out.slice(32, 64),
    id: CommonUtils.bytesAsHex(out.slice(0, 32))
  };
  if (size > 64) {
    result.extra = out.slice(64);
  }

  return result;
}





this.Intl = function Intl() {
  
  this._accepted = "";
  this._everRead = false;
  this._log = Log.repository.getLogger("Services.common.RESTRequest");
  this._log.level = Log.Level[Prefs.get("log.logger.rest.request")];
  this.init();
};

this.Intl.prototype = {
  init: function() {
    Services.prefs.addObserver("intl.accept_languages", this, false);
  },

  uninit: function() {
    Services.prefs.removeObserver("intl.accept_languages", this);
  },

  observe: function(subject, topic, data) {
    this.readPref();
  },

  readPref: function() {
    this._everRead = true;
    try {
      this._accepted = Services.prefs.getComplexValue(
        "intl.accept_languages", Ci.nsIPrefLocalizedString).data;
    } catch (err) {
      this._log.error("Error reading intl.accept_languages pref: " + CommonUtils.exceptionStr(err));
    }
  },

  get accept_languages() {
    if (!this._everRead) {
      this.readPref();
    }
    return this._accepted;
  },
};


let intl = null;
function getIntl() {
  if (!intl) {
    intl = new Intl();
  }
  return intl;
}

