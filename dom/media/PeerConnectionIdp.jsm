




this.EXPORTED_SYMBOLS = ['PeerConnectionIdp'];

const {classes: Cc, interfaces: Ci, utils: Cu} = Components;

Cu.import('resource://gre/modules/Services.jsm');
Cu.import('resource://gre/modules/XPCOMUtils.jsm');
XPCOMUtils.defineLazyModuleGetter(this, 'IdpSandbox',
  'resource://gre/modules/media/IdpSandbox.jsm');

function TimerResolver(resolve) {
  this.notify = resolve;
}
TimerResolver.prototype = {
  getInterface: function(iid) {
    return this.QueryInterface(iid);
  },
  QueryInterface: XPCOMUtils.generateQI([Ci.nsITimerCallback])
}
function delay(t) {
  return new Promise(resolve => {
    let timer = Cc['@mozilla.org/timer;1'].getService(Ci.nsITimer);
    timer.initWithCallback(new TimerResolver(resolve), t, 0); 
  });
}








function PeerConnectionIdp(timeout, warningFunc, dispatchErrorFunc) {
  this._timeout = timeout || 5000;
  this._warning = warningFunc;
  this._dispatchError = dispatchErrorFunc;

  this.assertion = null;
  this.provider = null;
}

(function() {
  PeerConnectionIdp._mLinePattern = new RegExp('^m=', 'm');
  
  let pattern = '^a=[iI][dD][eE][nN][tT][iI][tT][yY]:(\\S+)';
  PeerConnectionIdp._identityPattern = new RegExp(pattern, 'm');
  pattern = '^a=[fF][iI][nN][gG][eE][rR][pP][rR][iI][nN][tT]:(\\S+) (\\S+)';
  PeerConnectionIdp._fingerprintPattern = new RegExp(pattern, 'm');
})();

PeerConnectionIdp.prototype = {
  get enabled() {
    return !!this._idp;
  },

  setIdentityProvider: function(provider, protocol, username) {
    this.provider = provider;
    this.protocol = protocol || 'default';
    this.username = username;
    if (this._idp) {
      if (this._idp.isSame(provider, protocol)) {
        return; 
      }
      this._idp.stop();
    }
    this._idp = new IdpSandbox(provider, protocol);
  },

  close: function() {
    this.assertion = null;
    this.provider = null;
    this.protocol = null;
    if (this._idp) {
      this._idp.stop();
      this._idp = null;
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
    this._warning('RTC identity: ' + message, null, 0);
    this._dispatchError('idp' + type + 'error', args);
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

  _isValidAssertion: function(assertion) {
    return assertion && assertion.idp &&
      typeof assertion.idp.domain === 'string' &&
      (!assertion.idp.protocol ||
       typeof assertion.idp.protocol === 'string') &&
      typeof assertion.assertion === 'string';
  },

  _getIdentityFromSdp: function(sdp) {
    
    let idMatch;
    let mLineMatch = sdp.match(PeerConnectionIdp._mLinePattern);
    if (mLineMatch) {
      let sessionLevel = sdp.substring(0, mLineMatch.index);
      let idMatch = sessionLevel.match(PeerConnectionIdp._identityPattern);
    }
    if (!idMatch) {
      return; 
    }

    let assertion;
    try {
      assertion = JSON.parse(atob(idMatch[1]));
    } catch (e) {
      this.reportError('validation',
                       'invalid identity assertion: ' + e);
    }
    if (!this._isValidAssertion(assertion)) {
      this.reportError('validation', 'assertion missing' +
                       ' idp/idp.domain/assertion');
    }
    return assertion;
  },

  









  verifyIdentityFromSDP: function(sdp, origin) {
    let identity = this._getIdentityFromSdp(sdp);
    let fingerprints = this._getFingerprintsFromSdp(sdp);
    if (!identity || fingerprints.length <= 0) {
      return Promise.resolve();
    }

    this.setIdentityProvider(identity.idp.domain, identity.idp.protocol);
    return this._verifyIdentity(identity.assertion, fingerprints, origin);
  },

  






  _validateName: function(error, name) {
    if (typeof name !== 'string') {
      return error('name not a string');
    }
    let atIdx = name.indexOf('@');
    if (atIdx <= 0) {
      return error('missing authority in name from IdP');
    }

    
    let tail = name.substring(atIdx + 1);

    
    let provider = this.provider;
    let providerPortIdx = provider.indexOf(':');
    if (providerPortIdx > 0) {
      provider = provider.substring(0, providerPortIdx);
    }
    let idnService = Components.classes['@mozilla.org/network/idn-service;1'].
      getService(Components.interfaces.nsIIDNService);
    if (idnService.convertUTF8toACE(tail) !==
        idnService.convertUTF8toACE(provider)) {
      return error('name "' + identity.name +
            '" doesn\'t match IdP: "' + this.provider + '"');
    }
    return true;
  },

  




  _isValidVerificationResponse: function(validation, sdpFingerprints) {
    let error = msg => {
      this.reportError('validation', 'assertion validation failure: ' + msg);
      return false;
    };

    if (typeof validation !== 'object' ||
        typeof validation.contents !== 'string' ||
        typeof validation.identity !== 'string') {
      return error('no payload in validation response');
    }

    let fingerprints;
    try {
      fingerprints = JSON.parse(validation.contents).fingerprint;
    } catch (e) {
      return error('idp returned invalid JSON');
    }

    let isFingerprint = f =>
        (typeof f.digest === 'string') &&
        (typeof f.algorithm === 'string');
    if (!Array.isArray(fingerprints) || !fingerprints.every(isFingerprint)) {
      return error('fingerprints must be an array of objects' +
                   ' with digest and algorithm attributes');
    }

    let isSubsetOf = (outerSet, innerSet, comparator) => {
      return innerSet.every(i => {
        return outerSet.some(o => comparator(i, o));
      });
    };
    let compareFingerprints = (a, b) => {
      return (a.digest === b.digest) && (a.algorithm === b.algorithm);
    };
    if (!isSubsetOf(fingerprints, sdpFingerprints, compareFingerprints)) {
      return error('the fingerprints in SDP aren\'t covered by the assertion');
    }
    return this._validateName(error, validation.identity);
  },

  


  _verifyIdentity: function(assertion, fingerprints, origin) {
    let validationPromise = this._idp.start()
        .then(idp => idp.validateAssertion(assertion, origin));

    return this._safetyNet('validation', validationPromise)
      .then(validation => {
        if (validation &&
            this._isValidVerificationResponse(validation, fingerprints)) {
          return validation;
        }
      });
  },

  



  addIdentityAttribute: function(sdp) {
    if (!this.assertion) {
      return sdp;
    }

    
    let match = sdp.match(PeerConnectionIdp._mLinePattern);
    return sdp.substring(0, match.index) +
      'a=identity:' + this.assertion + '\r\n' +
      sdp.substring(match.index);
  },

  



  getIdentityAssertion: function(fingerprint) {
    if (!this.enabled) {
      this.reportError('assertion', 'no IdP set,' +
                       ' call setIdentityProvider() to set one');
      return Promise.resolve();
    }

    let [algorithm, digest] = fingerprint.split(' ', 2);
    let content = {
      fingerprint: [{
        algorithm: algorithm,
        digest: digest
      }]
    };
    let origin = Cu.getWebIDLCallerPrincipal().origin;

    let assertionPromise = this._idp.start()
        .then(idp => idp.generateAssertion(JSON.stringify(content),
                                           origin, this.username));

    return this._safetyNet('assertion', assertionPromise)
      .then(assertion => {
        if (this._isValidAssertion(assertion)) {
          
          this.assertion = btoa(JSON.stringify(assertion));
        } else {
          if (assertion) {
            
            
            this.reportError('assertion', 'invalid assertion generated');
          }
          this.assertion = null;
        }
        return this.assertion;
      });
  },

  




  _safetyNet: function(type, p) {
    let done = false; 
    let timeoutPromise = delay(this._timeout)
        .then(() => {
          if (!done) {
            this.reportError(type, 'IdP timed out');
          }
        });
    let realPromise = p
        .catch(e => this.reportError(type, 'error reported by IdP: ' + e.message))
        .then(result => {
          done = true;
          return result;
        });
    
    
    return Promise.race([realPromise, timeoutPromise]);
  }
};

this.PeerConnectionIdp = PeerConnectionIdp;
