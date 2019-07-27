




this.EXPORTED_SYMBOLS = ['PeerConnectionIdp'];

const {classes: Cc, interfaces: Ci, utils: Cu} = Components;

Cu.import('resource://gre/modules/Services.jsm');
Cu.import('resource://gre/modules/XPCOMUtils.jsm');
XPCOMUtils.defineLazyModuleGetter(this, 'IdpSandbox',
  'resource://gre/modules/media/IdpSandbox.jsm');







function PeerConnectionIdp(win, timeout) {
  this._win = win;
  this._timeout = timeout || 5000;

  this.provider = null;
  this._resetAssertion();
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

  _resetAssertion: function() {
    this.assertion = null;
    this.idpLoginUrl = null;
  },

  setIdentityProvider: function(provider, protocol, username) {
    this._resetAssertion();
    this.provider = provider;
    this.protocol = protocol || 'default';
    this.username = username;
    if (this._idp) {
      if (this._idp.isSame(provider, protocol)) {
        return; 
      }
      this._idp.stop();
    }
    this._idp = new IdpSandbox(provider, protocol, this._win.document);
  },

  
  start: function() {
    return this._idp.start()
      .catch(e => {
        throw new this._win.DOMException(e.message, 'IdpError');
      });
  },

  close: function() {
    this._resetAssertion();
    this.provider = null;
    this.protocol = null;
    if (this._idp) {
      this._idp.stop();
      this._idp = null;
    }
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
      idMatch = sessionLevel.match(PeerConnectionIdp._identityPattern);
    }
    if (!idMatch) {
      return; 
    }

    let assertion;
    try {
      assertion = JSON.parse(atob(idMatch[1]));
    } catch (e) {
      throw new this._win.DOMException('invalid identity assertion: ' + e,
                                       'InvalidSessionDescriptionError');
    }
    if (!this._isValidAssertion(assertion)) {
      throw new this._win.DOMException('assertion missing idp/idp.domain/assertion',
                                       'InvalidSessionDescriptionError');
    }
    return assertion;
  },

  









  verifyIdentityFromSDP: function(sdp, origin) {
    let identity = this._getIdentityFromSdp(sdp);
    let fingerprints = this._getFingerprintsFromSdp(sdp);
    if (!identity || fingerprints.length <= 0) {
      return this._win.Promise.resolve(); 
    }

    this.setIdentityProvider(identity.idp.domain, identity.idp.protocol);
    return this._verifyIdentity(identity.assertion, fingerprints, origin);
  },

  





  _validateName: function(name) {
    let error = msg => {
        throw new this._win.DOMException('assertion name error: ' + msg,
                                         'IdpError');
    };

    if (typeof name !== 'string') {
      error('name not a string');
    }
    let atIdx = name.indexOf('@');
    if (atIdx <= 0) {
      error('missing authority in name from IdP');
    }

    
    let tail = name.substring(atIdx + 1);

    
    let provider = this.provider;
    let providerPortIdx = provider.indexOf(':');
    if (providerPortIdx > 0) {
      provider = provider.substring(0, providerPortIdx);
    }
    let idnService = Components.classes['@mozilla.org/network/idn-service;1']
        .getService(Components.interfaces.nsIIDNService);
    if (idnService.convertUTF8toACE(tail) !==
        idnService.convertUTF8toACE(provider)) {
      error('name "' + identity.name +
            '" doesn\'t match IdP: "' + this.provider + '"');
    }
  },

  




  _checkValidation: function(validation, sdpFingerprints) {
    let error = msg => {
      throw new this._win.DOMException('IdP validation error: ' + msg,
                                       'IdpError');
    };

    if (!this.provider) {
      error('IdP closed');
    }

    if (typeof validation !== 'object' ||
        typeof validation.contents !== 'string' ||
        typeof validation.identity !== 'string') {
      error('no payload in validation response');
    }

    let fingerprints;
    try {
      fingerprints = JSON.parse(validation.contents).fingerprint;
    } catch (e) {
      error('invalid JSON');
    }

    let isFingerprint = f =>
        (typeof f.digest === 'string') &&
        (typeof f.algorithm === 'string');
    if (!Array.isArray(fingerprints) || !fingerprints.every(isFingerprint)) {
      error('fingerprints must be an array of objects' +
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
      error('the fingerprints must be covered by the assertion');
    }
    this._validateName(validation.identity);
    return validation;
  },

  


  _verifyIdentity: function(assertion, fingerprints, origin) {
    let p = this.start()
        .then(idp => this._wrapCrossCompartmentPromise(
          idp.validateAssertion(assertion, origin)))
        .then(validation => this._checkValidation(validation, fingerprints));

    return this._applyTimeout(p);
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

  





  getIdentityAssertion: function(fingerprint, origin) {
    if (!this.enabled) {
      throw new this._win.DOMException(
        'no IdP set, call setIdentityProvider() to set one', 'InvalidStateError');
    }

    let [algorithm, digest] = fingerprint.split(' ', 2);
    let content = {
      fingerprint: [{
        algorithm: algorithm,
        digest: digest
      }]
    };

    this._resetAssertion();
    let p = this.start()
        .then(idp => this._wrapCrossCompartmentPromise(
          idp.generateAssertion(JSON.stringify(content),
                                origin, this.username)))
        .then(assertion => {
          if (!this._isValidAssertion(assertion)) {
            throw new this._win.DOMException('IdP generated invalid assertion',
                                             'IdpError');
          }
          
          this.assertion = btoa(JSON.stringify(assertion));
          return this.assertion;
        });

    return this._applyTimeout(p);
  },

  




  _wrapCrossCompartmentPromise: function(sandboxPromise) {
    return new this._win.Promise((resolve, reject) => {
      sandboxPromise.then(
        result => resolve(Cu.cloneInto(result, this._win)),
        e => {
          let message = '' + (e.message || JSON.stringify(e) || 'IdP error');
          if (e.name === 'IdpLoginError') {
            if (typeof e.loginUrl === 'string') {
              this.idpLoginUrl = e.loginUrl;
            }
            reject(new this._win.DOMException(message, 'IdpLoginError'));
          } else {
            reject(new this._win.DOMException(message, 'IdpError'));
          }
        });
    });
  },

  




  _applyTimeout: function(p) {
    let timeout = new this._win.Promise(
      r => this._win.setTimeout(r, this._timeout))
        .then(() => {
          throw new this._win.DOMException('IdP timed out', 'IdpError');
        });
    return this._win.Promise.race([ timeout, p ]);
  }
};

this.PeerConnectionIdp = PeerConnectionIdp;
