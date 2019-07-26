



this.EXPORTED_SYMBOLS = ["fxAccounts", "FxAccounts"];

const {classes: Cc, interfaces: Ci, utils: Cu} = Components;

Cu.import("resource://gre/modules/Log.jsm");
Cu.import("resource://gre/modules/Promise.jsm");
Cu.import("resource://gre/modules/osfile.jsm");
Cu.import("resource://services-common/utils.js");
Cu.import("resource://services-crypto/utils.js");
Cu.import("resource://gre/modules/Services.jsm");
Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://gre/modules/Timer.jsm");
Cu.import("resource://gre/modules/Task.jsm");
Cu.import("resource://gre/modules/FxAccountsClient.jsm");
Cu.import("resource://gre/modules/FxAccountsCommon.js");
Cu.import("resource://gre/modules/FxAccountsUtils.jsm");

XPCOMUtils.defineLazyModuleGetter(this, "jwcrypto",
  "resource://gre/modules/identity/jwcrypto.jsm");


let publicProperties = [
  "getAccountsSignInURI",
  "getAccountsURI",
  "getAssertion",
  "getKeys",
  "getSignedInUser",
  "loadAndPoll",
  "localtimeOffsetMsec",
  "now",
  "promiseAccountsForceSigninURI",
  "resendVerificationEmail",
  "setSignedInUser",
  "signOut",
  "version",
  "whenVerified"
];




this.FxAccounts = function (mockInternal) {
  let internal = new FxAccountsInternal();
  let external = {};

  
  let prototype = FxAccountsInternal.prototype;
  let options = {keys: publicProperties, bind: internal};
  FxAccountsUtils.copyObjectProperties(prototype, external, options);

  
  if (mockInternal && !mockInternal.onlySetInternal) {
    FxAccountsUtils.copyObjectProperties(mockInternal, internal);
  }

  if (mockInternal) {
    
    external.internal = internal;
  }

  return Object.freeze(external);
}




function FxAccountsInternal() {
  this.cert = null;
  this.keyPair = null;
  this.signedInUser = null;
  this.version = DATA_FORMAT_VERSION;

  
  this.POLL_STEP = POLL_STEP;
  this.POLL_SESSION = POLL_SESSION;
  
  

  
  
  
  
  
  
  
  
  
  this.whenVerifiedPromise = null;
  this.whenKeysReadyPromise = null;
  this.currentTimer = null;
  this.generationCount = 0;

  this.fxAccountsClient = new FxAccountsClient();

  
  
  this.signedInUserStorage = new JSONStorage({
    filename: DEFAULT_STORAGE_FILENAME,
    baseDir: OS.Constants.Path.profileDir,
  });
}




FxAccountsInternal.prototype = {

  


  version: DATA_FORMAT_VERSION,

  



  now: function() {
    return this.fxAccountsClient.now();
  },

  







  get localtimeOffsetMsec() {
    return this.fxAccountsClient.localtimeOffsetMsec;
  },

  


  checkEmailStatus: function checkEmailStatus(sessionToken) {
    return this.fxAccountsClient.recoveryEmailStatus(sessionToken);
  },

  


  fetchKeys: function fetchKeys(keyFetchToken) {
    log.debug("fetchKeys: " + keyFetchToken);
    return this.fxAccountsClient.accountKeys(keyFetchToken);
  },

  
  
  
  
  
  
  
  
  
  
  

  
















  getSignedInUser: function getSignedInUser() {
    return this.getUserAccountData().then(data => {
      if (!data) {
        return null;
      }
      if (!this.isUserEmailVerified(data)) {
        
        
        
        this.startVerifiedCheck(data);
      }
      return data;
    });
  },

  


















  setSignedInUser: function setSignedInUser(credentials) {
    log.debug("setSignedInUser - aborting any existing flows");
    this.abortExistingFlow();

    let record = {version: this.version, accountData: credentials};
    
    this.signedInUser = JSON.parse(JSON.stringify(record));

    
    
    return this.signedInUserStorage.set(record).then(() => {
      this.notifyObservers(ONLOGIN_NOTIFICATION);
      if (!this.isUserEmailVerified(credentials)) {
        this.startVerifiedCheck(credentials);
      }
    });
  },

  



  getAssertion: function getAssertion(audience) {
    log.debug("enter getAssertion()");
    let mustBeValidUntil = this.now() + ASSERTION_LIFETIME;
    return this.getUserAccountData().then(data => {
      if (!data) {
        
        return null;
      }
      if (!this.isUserEmailVerified(data)) {
        
        return null;
      }
      return this.getKeyPair(mustBeValidUntil).then(keyPair => {
        return this.getCertificate(data, keyPair, mustBeValidUntil)
          .then(cert => {
            return this.getAssertionFromCert(data, keyPair, cert, audience);
          });
      });
    });
  },

  



  resendVerificationEmail: function resendVerificationEmail() {
    return this.getSignedInUser().then(data => {
      
      
      
      if (data) {
        this.pollEmailStatus(data.sessionToken, "start");
        return this.fxAccountsClient.resendVerificationEmail(data.sessionToken);
      }
      throw new Error("Cannot resend verification email; no signed-in user");
    });
  },

  


  abortExistingFlow: function abortExistingFlow() {
    if (this.currentTimer) {
      log.debug("Polling aborted; Another user signing in");
      clearTimeout(this.currentTimer);
      this.currentTimer = 0;
    }
    this.generationCount++;
    log.debug("generationCount: " + this.generationCount);

    if (this.whenVerifiedPromise) {
      this.whenVerifiedPromise.reject(
        new Error("Verification aborted; Another user signing in"));
      this.whenVerifiedPromise = null;
    }

    if (this.whenKeysReadyPromise) {
      this.whenKeysReadyPromise.reject(
        new Error("KeyFetch aborted; Another user signing in"));
      this.whenKeysReadyPromise = null;
    }
  },

  signOut: function signOut() {
    this.abortExistingFlow();
    this.signedInUser = null; 
    return this.signedInUserStorage.set(null).then(() => {
      this.notifyObservers(ONLOGOUT_NOTIFICATION);
    });
  },

  


















  getKeys: function() {
    return this.getUserAccountData().then((data) => {
      if (!data) {
        throw new Error("Can't get keys; User is not signed in");
      }
      if (data.kA && data.kB) {
        return data;
      }
      if (!this.whenKeysReadyPromise) {
        this.whenKeysReadyPromise = Promise.defer();
        this.fetchAndUnwrapKeys(data.keyFetchToken).then(data => {
          if (this.whenKeysReadyPromise) {
            this.whenKeysReadyPromise.resolve(data);
          }
        });
      }
      return this.whenKeysReadyPromise.promise;
    });
   },

  fetchAndUnwrapKeys: function(keyFetchToken) {
    log.debug("fetchAndUnwrapKeys: token: " + keyFetchToken);
    return Task.spawn(function* task() {
      
      if (!keyFetchToken) {
        yield this.signOut();
        return null;
      }
      let myGenerationCount = this.generationCount;

      let {kA, wrapKB} = yield this.fetchKeys(keyFetchToken);

      let data = yield this.getUserAccountData();

      
      if (data.keyFetchToken !== keyFetchToken) {
        throw new Error("Signed in user changed while fetching keys!");
      }

      
      
      let kB_hex = CryptoUtils.xor(CommonUtils.hexToBytes(data.unwrapBKey),
                                   wrapKB);

      log.debug("kB_hex: " + kB_hex);
      data.kA = CommonUtils.bytesAsHex(kA);
      data.kB = CommonUtils.bytesAsHex(kB_hex);

      delete data.keyFetchToken;

      log.debug("Keys Obtained: kA=" + data.kA + ", kB=" + data.kB);

      
      
      if (this.generationCount !== myGenerationCount) {
        return null;
      }

      yield this.setUserAccountData(data);

      
      
      
      this.notifyObservers(ONVERIFIED_NOTIFICATION);
      return data;
    }.bind(this));
  },

  getAssertionFromCert: function(data, keyPair, cert, audience) {
    log.debug("getAssertionFromCert");
    let payload = {};
    let d = Promise.defer();
    let options = {
      localtimeOffsetMsec: this.localtimeOffsetMsec,
      now: this.now()
    };
    
    
    jwcrypto.generateAssertion(cert, keyPair, audience, options, (err, signed) => {
      if (err) {
        log.error("getAssertionFromCert: " + err);
        d.reject(err);
      } else {
        log.debug("getAssertionFromCert returning signed: " + signed);
        d.resolve(signed);
      }
    });
    return d.promise;
  },

  getCertificate: function(data, keyPair, mustBeValidUntil) {
    log.debug("getCertificate" + JSON.stringify(this.signedInUserStorage));
    
    if (this.cert && this.cert.validUntil > mustBeValidUntil) {
      log.debug(" getCertificate already had one");
      return Promise.resolve(this.cert.cert);
    }
    
    let willBeValidUntil = this.now() + CERT_LIFETIME;
    return this.getCertificateSigned(data.sessionToken,
                                     keyPair.serializedPublicKey,
                                     CERT_LIFETIME)
      .then((cert) => {
        this.cert = {
          cert: cert,
          validUntil: willBeValidUntil
        };
        return cert;
      }
    );
  },

  getCertificateSigned: function(sessionToken, serializedPublicKey, lifetime) {
    log.debug("getCertificateSigned: " + sessionToken + " " + serializedPublicKey);
    return this.fxAccountsClient.signCertificate(sessionToken,
                                                 JSON.parse(serializedPublicKey),
                                                 lifetime);
  },

  getKeyPair: function(mustBeValidUntil) {
    if (this.keyPair && (this.keyPair.validUntil > mustBeValidUntil)) {
      log.debug("getKeyPair: already have a keyPair");
      return Promise.resolve(this.keyPair.keyPair);
    }
    
    let willBeValidUntil = this.now() + KEY_LIFETIME;
    let d = Promise.defer();
    jwcrypto.generateKeyPair("DS160", (err, kp) => {
      if (err) {
        d.reject(err);
      } else {
        this.keyPair = {
          keyPair: kp,
          validUntil: willBeValidUntil
        };
        log.debug("got keyPair");
        delete this.cert;
        d.resolve(this.keyPair.keyPair);
      }
    });
    return d.promise;
  },

  getUserAccountData: function() {
    
    if (this.signedInUser) {
      return Promise.resolve(this.signedInUser.accountData);
    }

    let deferred = Promise.defer();
    this.signedInUserStorage.get()
      .then((user) => {
        log.debug("getUserAccountData -> " + JSON.stringify(user));
        if (user && user.version == this.version) {
          log.debug("setting signed in user");
          this.signedInUser = user;
        }
        deferred.resolve(user ? user.accountData : null);
      },
      (err) => {
        if (err instanceof OS.File.Error && err.becauseNoSuchFile) {
          
          
          deferred.resolve(null);
        } else {
          deferred.reject(err);
        }
      }
    );

    return deferred.promise;
  },

  isUserEmailVerified: function isUserEmailVerified(data) {
    return !!(data && data.verified);
  },

  


  loadAndPoll: function() {
    return this.getUserAccountData()
      .then(data => {
        if (data && !this.isUserEmailVerified(data)) {
          this.pollEmailStatus(data.sessionToken, "start");
        }
        return data;
      });
  },

  startVerifiedCheck: function(data) {
    log.debug("startVerifiedCheck " + JSON.stringify(data));
    
    
    
    
    
    
    return this.whenVerified(data)
      .then((data) => this.getKeys(data));
  },

  whenVerified: function(data) {
    if (data.verified) {
      log.debug("already verified");
      return Promise.resolve(data);
    }
    if (!this.whenVerifiedPromise) {
      log.debug("whenVerified promise starts polling for verified email");
      this.pollEmailStatus(data.sessionToken, "start");
    }
    return this.whenVerifiedPromise.promise;
  },

  notifyObservers: function(topic) {
    log.debug("Notifying observers of " + topic);
    Services.obs.notifyObservers(null, topic, null);
  },

  pollEmailStatus: function pollEmailStatus(sessionToken, why) {
    let myGenerationCount = this.generationCount;
    log.debug("entering pollEmailStatus: " + why + " " + myGenerationCount);
    if (why == "start") {
      
      
      
      this.pollTimeRemaining = this.POLL_SESSION;
      if (!this.whenVerifiedPromise) {
        this.whenVerifiedPromise = Promise.defer();
      }
    }

    this.checkEmailStatus(sessionToken)
      .then((response) => {
        log.debug("checkEmailStatus -> " + JSON.stringify(response));
        
        
        if (this.generationCount !== myGenerationCount) {
          log.debug("generation count differs from " + this.generationCount + " - aborting");
          log.debug("sessionToken on abort is " + sessionToken);
          return;
        }

        if (response && response.verified) {
          
          
          this.getUserAccountData()
            .then((data) => {
              data.verified = true;
              return this.setUserAccountData(data);
            })
            .then((data) => {
              
              if (this.whenVerifiedPromise) {
                this.whenVerifiedPromise.resolve(data);
                delete this.whenVerifiedPromise;
              }
            });
        } else {
          log.debug("polling with step = " + this.POLL_STEP);
          this.pollTimeRemaining -= this.POLL_STEP;
          log.debug("time remaining: " + this.pollTimeRemaining);
          if (this.pollTimeRemaining > 0) {
            this.currentTimer = setTimeout(() => {
              this.pollEmailStatus(sessionToken, "timer")}, this.POLL_STEP);
            log.debug("started timer " + this.currentTimer);
          } else {
            if (this.whenVerifiedPromise) {
              this.whenVerifiedPromise.reject(
                new Error("User email verification timed out.")
              );
              delete this.whenVerifiedPromise;
            }
          }
        }
      });
    },

  setUserAccountData: function(accountData) {
    return this.signedInUserStorage.get().then(record => {
      record.accountData = accountData;
      this.signedInUser = record;
      return this.signedInUserStorage.set(record)
        .then(() => accountData);
    });
  },

  
  getAccountsURI: function() {
    let url = Services.urlFormatter.formatURLPref("identity.fxaccounts.remote.uri");
    if (!/^https:/.test(url)) { 
      throw new Error("Firefox Accounts server must use HTTPS");
    }
    return url;
  },

  
  getAccountsSignInURI: function() {
    let url = Services.urlFormatter.formatURLPref("identity.fxaccounts.remote.signin.uri");
    if (!/^https:/.test(url)) { 
      throw new Error("Firefox Accounts server must use HTTPS");
    }
    return url;
  },

  
  
  promiseAccountsForceSigninURI: function() {
    let url = Services.urlFormatter.formatURLPref("identity.fxaccounts.remote.force_auth.uri");
    if (!/^https:/.test(url)) { 
      throw new Error("Firefox Accounts server must use HTTPS");
    }
    
    return this.getSignedInUser().then(accountData => {
      if (!accountData) {
        return null;
      }
      let newQueryPortion = url.indexOf("?") == -1 ? "?" : "&";
      newQueryPortion += "email=" + encodeURIComponent(accountData.email);
      return url + newQueryPortion;
    });
  }
};












function JSONStorage(options) {
  this.baseDir = options.baseDir;
  this.path = OS.Path.join(options.baseDir, options.filename);
};

JSONStorage.prototype = {
  set: function(contents) {
    return OS.File.makeDir(this.baseDir, {ignoreExisting: true})
      .then(CommonUtils.writeJSON.bind(null, contents, this.path));
  },

  get: function() {
    return CommonUtils.readJSON(this.path);
  }
};


XPCOMUtils.defineLazyGetter(this, "fxAccounts", function() {
  let a = new FxAccounts();

  
  
  a.loadAndPoll();

  return a;
});
