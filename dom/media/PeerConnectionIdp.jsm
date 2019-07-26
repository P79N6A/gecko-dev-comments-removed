




this.EXPORTED_SYMBOLS = ["PeerConnectionIdp"];

const {classes: Cc, interfaces: Ci, utils: Cu} = Components;

Cu.import("resource://gre/modules/Services.jsm");
Cu.import("resource://gre/modules/XPCOMUtils.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "IdpProxy",
  "resource://gre/modules/media/IdpProxy.jsm");








function PeerConnectionIdp(window, timeout, warningFunc) {
  this._win = window;
  this._timeout = timeout || 5000;
  this._warning = warningFunc;

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

  _getFingerprintFromSdp: function(sdp) {
    let sections = sdp.split(PeerConnectionIdp._mLinePattern);
    let attributes = sections.map(function(sect) {
      let m = sect.match(PeerConnectionIdp._fingerprintPattern);
      if (m) {
        let remainder = sect.substring(m.index + m[0].length);
        if (!remainder.match(PeerConnectionIdp._fingerprintPattern)) {
          return { algorithm: m[1], digest: m[2] };
        }
        this._warning("RTC identity: two fingerprint values in same media " +
            "section are not supported", null, 0);
        
        
        return "error";
      }
      
    }, this);

    let sessionLevel = attributes.shift();
    attributes = attributes.map(function(sectionLevel) {
      return sectionLevel || sessionLevel;
    });

    let first = attributes.shift();
    function sameAsFirst(attr) {
      return typeof attr === "object" &&
      first.algorithm === attr.algorithm &&
      first.digest === attr.digest;
    }

    if (typeof first === "object" && attributes.every(sameAsFirst)) {
      return first;
    }
    
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
        this._warning("RTC identity: invalid identity assertion: " + e, null, 0);
      } 
      if (typeof assertion.idp === "object" &&
          typeof assertion.idp.domain === "string" &&
          typeof assertion.assertion === "string") {
        return assertion;
      }
      this._warning("RTC identity: assertion missing idp/idp.domain/assertion",
                    null, 0);
    }
    
  },

  





  verifyIdentityFromSDP: function(sdp, callback) {
    let identity = this._getIdentityFromSdp(sdp);
    let fingerprint = this._getFingerprintFromSdp(sdp);
    
    
    if (!fingerprint || !identity) {
      callback(null);
      return;
    }
    this.setIdentityProvider(identity.idp.domain, identity.idp.protocol);

    this._verifyIdentity(identity.assertion, fingerprint, callback);
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
      var idnService = Components.classes["@mozilla.org/network/idn-service;1"].
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

  
  
  _checkVerifyResponse: function(
      message, fingerprint) {
    let warn = function(message) {
      this._warning("RTC identity: VERIFY error: " + message, null, 0);
    }.bind(this);

    try {
      let contents = JSON.parse(message.contents);
      if (typeof contents.fingerprint !== "object" ||
          typeof message.identity !== "object") {
        warn("fingerprint or identity not objects");
      } else if (contents.fingerprint.digest !== fingerprint.digest ||
          contents.fingerprint.algorithm !== fingerprint.algorithm) {
        warn("fingerprint does not match");
      } else {
        let error = this._validateName(message.identity.name);
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

  


  _verifyIdentity: function(
      assertion, fingerprint, callback) {
    function onVerification(message) {
      if (!message) {
        this._warning("RTC identity: verification failure", null, 0);
        callback(null);
        return;
      }
      if (this._checkVerifyResponse(message, fingerprint)) {
        callback(message);
      } else {
        callback(null);
      }
    }

    let request = {
      type: "VERIFY",
      message: assertion
    };
    this._sendToIdp(request, onVerification.bind(this));
  },

  





  appendIdentityToSDP: function(sdp, fingerprint, callback) {
    if (!this._idpchannel) {
      callback(sdp);
      return;
    }

    if (this.assertion) {
      callback(this.wrapSdp(sdp));
      return;
    }

    function onAssertion(assertion) {
      callback(this.wrapSdp(sdp), assertion);
    }

    this._getIdentityAssertion(fingerprint, onAssertion.bind(this));
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
      throw new Error("IdP not set");
    }

    this._getIdentityAssertion(fingerprint, callback);
  },

  _getIdentityAssertion: function(fingerprint, callback) {
    let [algorithm, digest] = fingerprint.split(" ");
    let message = {
      fingerprint: {
        algorithm: algorithm,
        digest: digest
      }
    };
    let request = {
      type: "SIGN",
      message: JSON.stringify(message),
      username: this.username
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

    this._sendToIdp(request, trapAssertion.bind(this));
  },

  


  _sendToIdp: function(request, callback) {
    
    
    
    request.origin = this._win.document.nodePrincipal.origin;

    this._idpchannel.send(request, this._wrapCallback(callback));
  },

  




  _wrapCallback: function(callback) {
    let timeout = this._win.setTimeout(function() {
      this._warning("RTC identity: IdP timeout for " + this._idpchannel + " " +
           (this._idpchannel.ready ? "[ready]" : "[not ready]"), null, 0);
      timeout = null;
      callback(null);
    }.bind(this), this._timeout);

    return function(message) {
      if (!timeout) {
        return;
      }
      this._win.clearTimeout(timeout);
      timeout = null;
      var content = null;
      if (message.type === "SUCCESS") {
        content = message.message;
      } else {
        this._warning("RTC Identity: received response of type '" +
            message.type + "' from IdP: " + message.message, null, 0);
      }
      callback(content);
    }.bind(this);
  }
};

this.PeerConnectionIdp = PeerConnectionIdp;
