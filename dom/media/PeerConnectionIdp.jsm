




this.EXPORTED_SYMBOLS = ["PeerConnectionIdp"];

const {classes: Cc, interfaces: Ci, utils: Cu} = Components;

Cu.import("resource://gre/modules/Services.jsm");
Cu.import("resource://gre/modules/XPCOMUtils.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "IdpProxy",
  "resource://gre/modules/media/IdpProxy.jsm");









function PeerConnectionIdp(window, timeout, warningFunc, dispatchEventFunc) {
  this._win = window;
  this._timeout = timeout || 5000;
  this._warning = warningFunc;
  this._dispatchEvent = dispatchEventFunc;

  this.assertion = null;
  this.provider = null;
}

(function() {
  PeerConnectionIdp._mLinePattern = new RegExp("^m=", "m");
  
  let pattern = "^a=[iI][dD][eE][nN][tT][iI][tT][yY]:(\\S+)";
  PeerConnectionIdp._identityPattern = new RegExp(pattern, "m");
  pattern = "^a=[fF][iI][nN][gG][eE][rR][pP][rR][iI][nN][tT]:(\\S+) (\\S+)";
  PeerConnectionIdp._fingerprintPattern = new RegExp(pattern, "m");
})();

PeerConnectionIdp.prototype = {
  setIdentityProvider: function(provider, protocol, username) {
    this.provider = provider;
    this.protocol = protocol;
    this.username = username;
    if (this._idpchannel) {
      if (this._idpchannel.isSame(provider, protocol)) {
        return;
      }
      this._idpchannel.close();
    }
    this._idpchannel = new IdpProxy(provider, protocol);
  },

  close: function() {
    this.assertion = null;
    this.provider = null;
    if (this._idpchannel) {
      this._idpchannel.close();
      this._idpchannel = null;
    }
  },

  



  reportError: function(type, message, extra) {
    let args = {
      idp: this.provider,
      protocol: this.protocol
    };
    if (extra) {
      Object.keys(extra).forEach(function(k) {
        args[k] = extra[k];
      });
    }
    this._warning("RTC identity: " + message, null, 0);
    let ev = new this._win.RTCPeerConnectionIdentityErrorEvent('idp' + type + 'error', args);
    this._dispatchEvent(ev);
  },

  _getFingerprintsFromSdp: function(sdp) {
    let fingerprints = {};
    let m = sdp.match(PeerConnectionIdp._fingerprintPattern);
    while (m) {
      fingerprints[m[0]] = { algorithm: m[1], digest: m[2] };
      sdp = sdp.substring(m.index + m[0].length);
      m = sdp.match(PeerConnectionIdp._fingerprintPattern);
    }

    return Object.keys(fingerprints).map(k => fingerprints[k]);
  },

  _getIdentityFromSdp: function(sdp) {
    
    let mLineMatch = sdp.match(PeerConnectionIdp._mLinePattern);
    let sessionLevel = sdp.substring(0, mLineMatch.index);
    let idMatch = sessionLevel.match(PeerConnectionIdp._identityPattern);
    if (idMatch) {
      let assertion = {};
      try {
        assertion = JSON.parse(atob(idMatch[1]));
      } catch (e) {
        this.reportError("validation",
                         "invalid identity assertion: " + e);
      } 
      if (typeof assertion.idp === "object" &&
          typeof assertion.idp.domain === "string" &&
          typeof assertion.assertion === "string") {
        return assertion;
      }

      this.reportError("validation", "assertion missing" +
                       " idp/idp.domain/assertion");
    }
    
  },

  





  verifyIdentityFromSDP: function(sdp, origin, callback) {
    let identity = this._getIdentityFromSdp(sdp);
    let fingerprints = this._getFingerprintsFromSdp(sdp);
    
    
    if (!identity || fingerprints.length <= 0) {
      callback(null);
      return;
    }

    this.setIdentityProvider(identity.idp.domain, identity.idp.protocol);
    this._verifyIdentity(identity.assertion, fingerprints, origin, callback);
  },

  





  _validateName: function(name) {
    if (typeof name !== "string") {
      return "name not a string";
    }
    let atIdx = name.indexOf("@");
    if (atIdx > 0) {
      
      let tail = name.substring(atIdx + 1);

      
      let provider = this.provider;
      let providerPortIdx = provider.indexOf(":");
      if (providerPortIdx > 0) {
        provider = provider.substring(0, providerPortIdx);
      }
      let idnService = Components.classes["@mozilla.org/network/idn-service;1"].
        getService(Components.interfaces.nsIIDNService);
      if (idnService.convertUTF8toACE(tail) !==
          idnService.convertUTF8toACE(provider)) {
        return "name '" + identity.name +
            "' doesn't match IdP: '" + this.provider + "'";
      }
      return null;
    }
    return "missing authority in name from IdP";
  },

  
  
  _checkVerifyResponse: function(message, fingerprints) {
    let warn = msg => {
      this.reportError("validation",
                       "assertion validation failure: " + msg);
    };

    let isSubsetOf = (outer, inner, cmp) => {
      return inner.some(i => {
        return !outer.some(o => cmp(i, o));
      });
    };
    let compareFingerprints = (a, b) => {
      return (a.digest === b.digest) && (a.algorithm === b.algorithm);
    };

    try {
      let contents = JSON.parse(message.contents);
      if (!Array.isArray(contents.fingerprint)) {
        warn("fingerprint is not an array");
      } else if (isSubsetOf(contents.fingerprint, fingerprints,
                            compareFingerprints)) {
        warn("fingerprints in SDP aren't a subset of those in the assertion");
      } else {
        let error = this._validateName(message.identity);
        if (error) {
          warn(error);
        } else {
          return true;
        }
      }
    } catch(e) {
      warn("invalid JSON in content");
    }
    return false;
  },

  


  _verifyIdentity: function(assertion, fingerprints, origin, callback) {
    function onVerification(message) {
      if (message && this._checkVerifyResponse(message, fingerprints)) {
        callback(message);
      } else {
        this._warning("RTC identity: assertion validation failure", null, 0);
        callback(null);
      }
    }

    let request = {
      type: "VERIFY",
      message: assertion,
      origin: origin
    };
    this._sendToIdp(request, "validation", onVerification.bind(this));
  },

  





  appendIdentityToSDP: function(sdp, fingerprint, origin, callback) {
    let onAssertion = function() {
      callback(this.wrapSdp(sdp), this.assertion);
    }.bind(this);

    if (!this._idpchannel || this.assertion) {
      onAssertion();
      return;
    }

    this._getIdentityAssertion(fingerprint, origin, onAssertion);
  },

  


  wrapSdp: function(sdp) {
    if (!this.assertion) {
      return sdp;
    }

    
    let match = sdp.match(PeerConnectionIdp._mLinePattern);
    return sdp.substring(0, match.index) +
      "a=identity:" + this.assertion + "\r\n" +
      sdp.substring(match.index);
  },

  getIdentityAssertion: function(fingerprint, callback) {
    if (!this._idpchannel) {
      this.reportError("assertion", "IdP not set");
      callback(null);
      return;
    }

    let origin = Cu.getWebIDLCallerPrincipal().origin;
    this._getIdentityAssertion(fingerprint, origin, callback);
  },

  _getIdentityAssertion: function(fingerprint, origin, callback) {
    let [algorithm, digest] = fingerprint.split(" ", 2);
    let message = {
      fingerprint: [{
        algorithm: algorithm,
        digest: digest
      }]
    };
    let request = {
      type: "SIGN",
      message: JSON.stringify(message),
      username: this.username,
      origin: origin
    };

    
    function trapAssertion(assertion) {
      if (!assertion) {
        this._warning("RTC identity: assertion generation failure", null, 0);
        this.assertion = null;
      } else {
        this.assertion = btoa(JSON.stringify(assertion));
      }
      callback(this.assertion);
    }

    this._sendToIdp(request, "assertion", trapAssertion.bind(this));
  },

  





  _sendToIdp: function(request, type, callback) {
    this._idpchannel.send(request, this._wrapCallback(type, callback));
  },

  _reportIdpError: function(type, message) {
    let args = {};
    let msg = "";
    if (message.type === "ERROR") {
      msg = message.error;
    } else {
      msg = JSON.stringify(message.message);
      if (message.type === "LOGINNEEDED") {
        args.loginUrl = message.loginUrl;
      }
    }
    this.reportError(type, "received response of type '" +
                     message.type + "' from IdP: " + msg, args);
  },

  




  _wrapCallback: function(type, callback) {
    let timeout = this._win.setTimeout(function() {
      this.reportError(type, "IdP timeout for " + this._idpchannel + " " +
                       (this._idpchannel.ready ? "[ready]" : "[not ready]"));
      timeout = null;
      callback(null);
    }.bind(this), this._timeout);

    return function(message) {
      if (!timeout) {
        return;
      }
      this._win.clearTimeout(timeout);
      timeout = null;

      let content = null;
      if (message.type === "SUCCESS") {
        content = message.message;
      } else {
        this._reportIdpError(type, message);
      }
      callback(content);
    }.bind(this);
  }
};

this.PeerConnectionIdp = PeerConnectionIdp;
