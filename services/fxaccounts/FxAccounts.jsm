



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

XPCOMUtils.defineLazyModuleGetter(this, "jwcrypto",
                                  "resource://gre/modules/identity/jwcrypto.jsm");

InternalMethods = function(mock) {
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

  if (mock) { 
    Object.keys(mock).forEach((prop) => {
      log.debug("InternalMethods: mocking: " + prop);
      this[prop] = mock[prop];
    });
  }
  if (!this.signedInUserStorage) {
    
    
    
    this.signedInUserStorage = new JSONStorage({
      filename: DEFAULT_STORAGE_FILENAME,
      baseDir: OS.Constants.Path.profileDir,
    });
  }
}
InternalMethods.prototype = {

  


  checkEmailStatus: function checkEmailStatus(sessionToken) {
    return this.fxAccountsClient.recoveryEmailStatus(sessionToken);
  },

  


  fetchKeys: function fetchKeys(keyFetchToken) {
    log.debug("fetchKeys: " + keyFetchToken);
    return this.fxAccountsClient.accountKeys(keyFetchToken);
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
      this.notifyObservers("fxaccounts:onlogout");
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
        this.fetchAndUnwrapKeys(data.keyFetchToken)
          .then((data) => {
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
        yield internal.signOut();
        return null;
      }
      let myGenerationCount = internal.generationCount;

      let {kA, wrapKB} = yield internal.fetchKeys(keyFetchToken);

      let data = yield internal.getUserAccountData();

      
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

      
      
      if (internal.generationCount !== myGenerationCount) {
        return null;
      }

      yield internal.setUserAccountData(data);

      
      
      
      internal.notifyObservers("fxaccounts:onlogin");
      return data;
    }.bind(this));
  },

  getAssertionFromCert: function(data, keyPair, cert, audience) {
    log.debug("getAssertionFromCert");
    let payload = {};
    let d = Promise.defer();
    
    
    jwcrypto.generateAssertion(cert, keyPair, audience, function(err, signed) {
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
    return !!(data && data.isVerified);
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
    if (data.isVerified) {
      log.debug("already verified");
      return Promise.resolve(data);
    }
    if (!this.whenVerifiedPromise) {
      this.whenVerifiedPromise = Promise.defer();
      log.debug("whenVerified promise starts polling for verified email");
      this.pollEmailStatus(data.sessionToken, "start");
    }
    return this.whenVerifiedPromise.promise;
  },

  notifyObservers: function(topic) {
    log.debug("Notifying observers of user login");
    Services.obs.notifyObservers(null, topic, null);
  },

  




  now: function() {
    return Date.now();
  },

  pollEmailStatus: function pollEmailStatus(sessionToken, why) {
    let myGenerationCount = this.generationCount;
    log.debug("entering pollEmailStatus: " + why + " " + myGenerationCount);
    if (why == "start") {
      if (this.currentTimer) {
        
        
        throw new Error("Already polling for email status");
      }
      this.pollTimeRemaining = this.POLL_SESSION;
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
              data.isVerified = true;
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
    return this.signedInUserStorage.get().then((record) => {
      record.accountData = accountData;
      this.signedInUser = record;
      return this.signedInUserStorage.set(record)
        .then(() => accountData);
    });
  }
};

let internal = null;











this.FxAccounts = function(mockInternal) {
  let mocks = mockInternal;
  if (mocks && mocks.onlySetInternal) {
    mocks = null;
  }
  internal = new InternalMethods(mocks);
  if (mockInternal) {
    
    this.internal = internal;
  }
}
this.FxAccounts.prototype = Object.freeze({
  version: DATA_FORMAT_VERSION,

  
  
  
  
  
  
  
  
  
  
  

  
















  setSignedInUser: function setSignedInUser(credentials) {
    log.debug("setSignedInUser - aborting any existing flows");
    internal.abortExistingFlow();

    let record = {version: this.version, accountData: credentials };
    
    internal.signedInUser = JSON.parse(JSON.stringify(record));

    
    
    return internal.signedInUserStorage.set(record)
      .then(() => {
        if (!internal.isUserEmailVerified(credentials)) {
          internal.startVerifiedCheck(credentials);
        }
      });
  },

  














  getSignedInUser: function getSignedInUser() {
    return internal.getUserAccountData()
      .then((data) => {
        if (!data) {
          return null;
        }
        if (!internal.isUserEmailVerified(data)) {
          
          
          internal.startVerifiedCheck(data);
        }
        return data;
      });
  },

  



  getAssertion: function getAssertion(audience) {
    log.debug("enter getAssertion()");
    let mustBeValidUntil = internal.now() + ASSERTION_LIFETIME;
    return internal.getUserAccountData()
      .then((data) => {
        if (!data) {
          
          return null;
        }
        if (!internal.isUserEmailVerified(data)) {
          
          return null;
        }
        return internal.getKeyPair(mustBeValidUntil)
          .then((keyPair) => {
            return internal.getCertificate(data, keyPair, mustBeValidUntil)
              .then((cert) => {
                return internal.getAssertionFromCert(data, keyPair,
                                                     cert, audience)
              });
          });
      });
  },

  





  signOut: function signOut() {
    return internal.signOut();
  },

  
  getAccountsURI: function() {
    let url = Services.urlFormatter.formatURLPref("firefox.accounts.remoteUrl");
    if (!/^https:/.test(url)) { 
      throw new Error("Firefox Accounts server must use HTTPS");
    }
    return url;
  }
});












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

  
  
  internal.loadAndPoll();

  return a;
});

