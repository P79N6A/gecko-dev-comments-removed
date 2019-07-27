



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
Cu.import("resource://gre/modules/FxAccountsCommon.js");

XPCOMUtils.defineLazyModuleGetter(this, "FxAccountsClient",
  "resource://gre/modules/FxAccountsClient.jsm");

XPCOMUtils.defineLazyModuleGetter(this, "jwcrypto",
  "resource://gre/modules/identity/jwcrypto.jsm");

XPCOMUtils.defineLazyModuleGetter(this, "FxAccountsOAuthGrantClient",
  "resource://gre/modules/FxAccountsOAuthGrantClient.jsm");

XPCOMUtils.defineLazyModuleGetter(this, "FxAccountsProfile",
  "resource://gre/modules/FxAccountsProfile.jsm");


let publicProperties = [
  "accountStatus",
  "getAccountsClient",
  "getAccountsSignInURI",
  "getAccountsSignUpURI",
  "getAssertion",
  "getKeys",
  "getSignedInUser",
  "getOAuthToken",
  "getSignedInUserProfile",
  "loadAndPoll",
  "localtimeOffsetMsec",
  "now",
  "promiseAccountsForceSigninURI",
  "promiseAccountsChangeProfileURI",
  "resendVerificationEmail",
  "setSignedInUser",
  "signOut",
  "version",
  "whenVerified"
];



















let AccountState = function(fxaInternal) {
  this.fxaInternal = fxaInternal;
};

AccountState.prototype = {
  cert: null,
  keyPair: null,
  signedInUser: null,
  whenVerifiedDeferred: null,
  whenKeysReadyDeferred: null,
  profile: null,

  get isCurrent() this.fxaInternal && this.fxaInternal.currentAccountState === this,

  abort: function() {
    if (this.whenVerifiedDeferred) {
      this.whenVerifiedDeferred.reject(
        new Error("Verification aborted; Another user signing in"));
      this.whenVerifiedDeferred = null;
    }

    if (this.whenKeysReadyDeferred) {
      this.whenKeysReadyDeferred.reject(
        new Error("Verification aborted; Another user signing in"));
      this.whenKeysReadyDeferred = null;
    }

    this.cert = null;
    this.keyPair = null;
    this.signedInUser = null;
    this.fxaInternal = null;
    this.initProfilePromise = null;

    if (this.profile) {
      this.profile.tearDown();
      this.profile = null;
    }
  },

  getUserAccountData: function() {
    
    if (this.signedInUser) {
      return this.resolve(this.signedInUser.accountData);
    }

    return this.fxaInternal.signedInUserStorage.get().then(
      user => {
        if (logPII) {
          
          
          log.debug("getUserAccountData -> " + JSON.stringify(user));
        }
        if (user && user.version == this.version) {
          log.debug("setting signed in user");
          this.signedInUser = user;
        }
        return this.resolve(user ? user.accountData : null);
      },
      err => {
        if (err instanceof OS.File.Error && err.becauseNoSuchFile) {
          
          
          return this.resolve(null);
        }
        return this.reject(err);
      }
    );
  },

  setUserAccountData: function(accountData) {
    return this.fxaInternal.signedInUserStorage.get().then(record => {
      if (!this.isCurrent) {
        return this.reject(new Error("Another user has signed in"));
      }
      record.accountData = accountData;
      this.signedInUser = record;
      return this.fxaInternal.signedInUserStorage.set(record)
        .then(() => this.resolve(accountData));
    });
  },


  getCertificate: function(data, keyPair, mustBeValidUntil) {
    if (logPII) {
      
      
      log.debug("getCertificate" + JSON.stringify(this.signedInUser));
    }
    
    if (this.cert && this.cert.validUntil > mustBeValidUntil) {
      log.debug(" getCertificate already had one");
      return this.resolve(this.cert.cert);
    }

    if (Services.io.offline) {
      return this.reject(new Error(ERROR_OFFLINE));
    }

    let willBeValidUntil = this.fxaInternal.now() + CERT_LIFETIME;
    return this.fxaInternal.getCertificateSigned(data.sessionToken,
                                                 keyPair.serializedPublicKey,
                                                 CERT_LIFETIME).then(
      cert => {
        log.debug("getCertificate got a new one: " + !!cert);
        this.cert = {
          cert: cert,
          validUntil: willBeValidUntil
        };
        return cert;
      }
    ).then(result => this.resolve(result));
  },

  getKeyPair: function(mustBeValidUntil) {
    
    
    
    
    
    let ignoreCachedAuthCredentials = false;
    try {
      ignoreCachedAuthCredentials = Services.prefs.getBoolPref("services.sync.debug.ignoreCachedAuthCredentials");
    } catch(e) {
      
    }
    if (!ignoreCachedAuthCredentials && this.keyPair && (this.keyPair.validUntil > mustBeValidUntil)) {
      log.debug("getKeyPair: already have a keyPair");
      return this.resolve(this.keyPair.keyPair);
    }
    
    let willBeValidUntil = this.fxaInternal.now() + KEY_LIFETIME;
    let d = Promise.defer();
    jwcrypto.generateKeyPair("DS160", (err, kp) => {
      if (err) {
        return this.reject(err);
      }
      this.keyPair = {
        keyPair: kp,
        validUntil: willBeValidUntil
      };
      log.debug("got keyPair");
      delete this.cert;
      d.resolve(this.keyPair.keyPair);
    });
    return d.promise.then(result => this.resolve(result));
  },

  
  getProfile: function () {
    return this.initProfile()
      .then(() => this.profile.getProfile());
  },

  
  initProfile: function () {

    let profileServerUrl = Services.urlFormatter.formatURLPref("identity.fxaccounts.remote.profile.uri");

    let oAuthOptions = {
      scope: "profile"
    };

    if (this.initProfilePromise) {
      return this.initProfilePromise;
    }

    this.initProfilePromise = this.fxaInternal.getOAuthToken(oAuthOptions)
      .then(token => {
        this.profile = new FxAccountsProfile(this, {
          profileServerUrl: profileServerUrl,
          token: token
        });
        this.initProfilePromise = null;
      })
      .then(null, err => {
        this.initProfilePromise = null;
        throw err;
      });

    return this.initProfilePromise;
  },

  resolve: function(result) {
    if (!this.isCurrent) {
      log.info("An accountState promise was resolved, but was actually rejected" +
               " due to a different user being signed in. Originally resolved" +
               " with: " + result);
      return Promise.reject(new Error("A different user signed in"));
    }
    return Promise.resolve(result);
  },

  reject: function(error) {
    
    
    
    
    if (!this.isCurrent) {
      log.info("An accountState promise was rejected, but we are ignoring that" +
               "reason and rejecting it due to a different user being signed in." +
               "Originally rejected with: " + error);
      return Promise.reject(new Error("A different user signed in"));
    }
    return Promise.reject(error);
  },

};


















function copyObjectProperties(from, to, opts = {}) {
  let keys = (opts && opts.keys) || Object.keys(from);
  let thisArg = (opts && opts.bind) || to;

  for (let prop of keys) {
    let desc = Object.getOwnPropertyDescriptor(from, prop);

    if (typeof(desc.value) == "function") {
      desc.value = desc.value.bind(thisArg);
    }

    if (desc.get) {
      desc.get = desc.get.bind(thisArg);
    }

    if (desc.set) {
      desc.set = desc.set.bind(thisArg);
    }

    Object.defineProperty(to, prop, desc);
  }
}




this.FxAccounts = function (mockInternal) {
  let internal = new FxAccountsInternal();
  let external = {};

  
  let prototype = FxAccountsInternal.prototype;
  let options = {keys: publicProperties, bind: internal};
  copyObjectProperties(prototype, external, options);

  
  if (mockInternal && !mockInternal.onlySetInternal) {
    copyObjectProperties(mockInternal, internal);
  }

  if (mockInternal) {
    
    external.internal = internal;
  }

  return Object.freeze(external);
}




function FxAccountsInternal() {
  this.version = DATA_FORMAT_VERSION;

  
  this.POLL_SESSION = POLL_SESSION;

  
  
  
  
  
  
  
  
  
  
  this.currentTimer = null;
  this.currentAccountState = new AccountState(this);

  
  
  
#if defined(MOZ_B2G)
  this.signedInUserStorage = new JSONStorage({
#else
  this.signedInUserStorage = new LoginManagerStorage({
#endif
    filename: DEFAULT_STORAGE_FILENAME,
    baseDir: OS.Constants.Path.profileDir,
  });
}




FxAccountsInternal.prototype = {

  


  version: DATA_FORMAT_VERSION,

  _fxAccountsClient: null,

  get fxAccountsClient() {
    if (!this._fxAccountsClient) {
      this._fxAccountsClient = new FxAccountsClient();
    }
    return this._fxAccountsClient;
  },

  



  now: function() {
    return this.fxAccountsClient.now();
  },

  getAccountsClient: function() {
    return this.fxAccountsClient;
  },

  







  get localtimeOffsetMsec() {
    return this.fxAccountsClient.localtimeOffsetMsec;
  },

  


  checkEmailStatus: function checkEmailStatus(sessionToken) {
    return this.fxAccountsClient.recoveryEmailStatus(sessionToken);
  },

  


  fetchKeys: function fetchKeys(keyFetchToken) {
    log.debug("fetchKeys: " + !!keyFetchToken);
    if (logPII) {
      log.debug("fetchKeys - the token is " + keyFetchToken);
    }
    return this.fxAccountsClient.accountKeys(keyFetchToken);
  },

  
  
  
  
  
  
  
  
  
  
  

  
















  getSignedInUser: function getSignedInUser() {
    let currentState = this.currentAccountState;
    return currentState.getUserAccountData().then(data => {
      if (!data) {
        return null;
      }
      if (!this.isUserEmailVerified(data)) {
        
        
        
        this.startVerifiedCheck(data);
      }
      return data;
    }).then(result => currentState.resolve(result));
  },

  




















  setSignedInUser: function setSignedInUser(credentials) {
    log.debug("setSignedInUser - aborting any existing flows");
    this.abortExistingFlow();

    let record = {version: this.version, accountData: credentials};
    let currentState = this.currentAccountState;
    
    currentState.signedInUser = JSON.parse(JSON.stringify(record));

    
    
    return this.signedInUserStorage.set(record).then(() => {
      this.notifyObservers(ONLOGIN_NOTIFICATION);
      if (!this.isUserEmailVerified(credentials)) {
        this.startVerifiedCheck(credentials);
      }
    }).then(result => currentState.resolve(result));
  },

  



  getAssertion: function getAssertion(audience) {
    log.debug("enter getAssertion()");
    let currentState = this.currentAccountState;
    let mustBeValidUntil = this.now() + ASSERTION_USE_PERIOD;
    return currentState.getUserAccountData().then(data => {
      if (!data) {
        
        return null;
      }
      if (!this.isUserEmailVerified(data)) {
        
        return null;
      }
      return currentState.getKeyPair(mustBeValidUntil).then(keyPair => {
        return currentState.getCertificate(data, keyPair, mustBeValidUntil)
          .then(cert => {
            return this.getAssertionFromCert(data, keyPair, cert, audience);
          });
      });
    }).then(result => currentState.resolve(result));
  },

  



  resendVerificationEmail: function resendVerificationEmail() {
    let currentState = this.currentAccountState;
    return this.getSignedInUser().then(data => {
      
      
      
      if (data) {
        this.pollEmailStatus(currentState, data.sessionToken, "start");
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
    this.currentAccountState.abort();
    this.currentAccountState = new AccountState(this);
  },

  accountStatus: function accountStatus() {
    return this.currentAccountState.getUserAccountData().then(data => {
      if (!data) {
        return false;
      }
      return this.fxAccountsClient.accountStatus(data.uid);
    });
  },

  signOut: function signOut(localOnly) {
    let currentState = this.currentAccountState;
    let sessionToken;
    return currentState.getUserAccountData().then(data => {
      
      sessionToken = data && data.sessionToken;
      return this._signOutLocal();
    }).then(() => {
      
      
      if (!localOnly) {
        
        
        Promise.resolve().then(() => {
          
          
          
          return this._signOutServer(sessionToken);
        }).then(null, err => {
          log.error("Error during remote sign out of Firefox Accounts: " + err);
        });
      }
    }).then(() => {
      this.notifyObservers(ONLOGOUT_NOTIFICATION);
    });
  },

  



  _signOutLocal: function signOutLocal() {
    this.abortExistingFlow();
    this.currentAccountState.signedInUser = null; 
    return this.signedInUserStorage.set(null);
  },

  _signOutServer: function signOutServer(sessionToken) {
    return this.fxAccountsClient.signOut(sessionToken);
  },

  


















  getKeys: function() {
    let currentState = this.currentAccountState;
    return currentState.getUserAccountData().then((userData) => {
      if (!userData) {
        throw new Error("Can't get keys; User is not signed in");
      }
      if (userData.kA && userData.kB) {
        return userData;
      }
      if (!currentState.whenKeysReadyDeferred) {
        currentState.whenKeysReadyDeferred = Promise.defer();
        if (userData.keyFetchToken) {
          this.fetchAndUnwrapKeys(userData.keyFetchToken).then(
            (dataWithKeys) => {
              if (!dataWithKeys.kA || !dataWithKeys.kB) {
                currentState.whenKeysReadyDeferred.reject(
                  new Error("user data missing kA or kB")
                );
                return;
              }
              currentState.whenKeysReadyDeferred.resolve(dataWithKeys);
            },
            (err) => {
              currentState.whenKeysReadyDeferred.reject(err);
            }
          );
        } else {
          currentState.whenKeysReadyDeferred.reject('No keyFetchToken');
        }
      }
      return currentState.whenKeysReadyDeferred.promise;
    }).then(result => currentState.resolve(result));
   },

  fetchAndUnwrapKeys: function(keyFetchToken) {
    if (logPII) {
      log.debug("fetchAndUnwrapKeys: token: " + keyFetchToken);
    }
    let currentState = this.currentAccountState;
    return Task.spawn(function* task() {
      
      if (!keyFetchToken) {
        log.warn("improper fetchAndUnwrapKeys() call: token missing");
        yield this.signOut();
        return null;
      }

      let {kA, wrapKB} = yield this.fetchKeys(keyFetchToken);

      let data = yield currentState.getUserAccountData();

      
      if (data.keyFetchToken !== keyFetchToken) {
        throw new Error("Signed in user changed while fetching keys!");
      }

      
      
      let kB_hex = CryptoUtils.xor(CommonUtils.hexToBytes(data.unwrapBKey),
                                   wrapKB);

      if (logPII) {
        log.debug("kB_hex: " + kB_hex);
      }
      data.kA = CommonUtils.bytesAsHex(kA);
      data.kB = CommonUtils.bytesAsHex(kB_hex);

      delete data.keyFetchToken;
      delete data.unwrapBKey;

      log.debug("Keys Obtained: kA=" + !!data.kA + ", kB=" + !!data.kB);
      if (logPII) {
        log.debug("Keys Obtained: kA=" + data.kA + ", kB=" + data.kB);
      }

      yield currentState.setUserAccountData(data);
      
      
      
      this.notifyObservers(ONVERIFIED_NOTIFICATION);
      return data;
    }.bind(this)).then(result => currentState.resolve(result));
  },

  getAssertionFromCert: function(data, keyPair, cert, audience) {
    log.debug("getAssertionFromCert");
    let payload = {};
    let d = Promise.defer();
    let options = {
      duration: ASSERTION_LIFETIME,
      localtimeOffsetMsec: this.localtimeOffsetMsec,
      now: this.now()
    };
    let currentState = this.currentAccountState;
    
    
    jwcrypto.generateAssertion(cert, keyPair, audience, options, (err, signed) => {
      if (err) {
        log.error("getAssertionFromCert: " + err);
        d.reject(err);
      } else {
        log.debug("getAssertionFromCert returning signed: " + !!signed);
        if (logPII) {
          log.debug("getAssertionFromCert returning signed: " + signed);
        }
        d.resolve(signed);
      }
    });
    return d.promise.then(result => currentState.resolve(result));
  },

  getCertificateSigned: function(sessionToken, serializedPublicKey, lifetime) {
    log.debug("getCertificateSigned: " + !!sessionToken + " " + !!serializedPublicKey);
    if (logPII) {
      log.debug("getCertificateSigned: " + sessionToken + " " + serializedPublicKey);
    }
    return this.fxAccountsClient.signCertificate(
      sessionToken,
      JSON.parse(serializedPublicKey),
      lifetime
    );
  },

  getUserAccountData: function() {
    return this.currentAccountState.getUserAccountData();
  },

  isUserEmailVerified: function isUserEmailVerified(data) {
    return !!(data && data.verified);
  },

  


  loadAndPoll: function() {
    let currentState = this.currentAccountState;
    return currentState.getUserAccountData()
      .then(data => {
        if (data && !this.isUserEmailVerified(data)) {
          this.pollEmailStatus(currentState, data.sessionToken, "start");
        }
        return data;
      });
  },

  startVerifiedCheck: function(data) {
    log.debug("startVerifiedCheck", data && data.verified);
    if (logPII) {
      log.debug("startVerifiedCheck with user data", data);
    }

    
    
    
    
    
    

    
    
    
    this.whenVerified(data).then(
      () => this.getKeys(),
      err => log.info("startVerifiedCheck promise was rejected: " + err)
    );
  },

  whenVerified: function(data) {
    let currentState = this.currentAccountState;
    if (data.verified) {
      log.debug("already verified");
      return currentState.resolve(data);
    }
    if (!currentState.whenVerifiedDeferred) {
      log.debug("whenVerified promise starts polling for verified email");
      this.pollEmailStatus(currentState, data.sessionToken, "start");
    }
    return currentState.whenVerifiedDeferred.promise.then(
      result => currentState.resolve(result)
    );
  },

  notifyObservers: function(topic, data) {
    log.debug("Notifying observers of " + topic);
    Services.obs.notifyObservers(null, topic, data);
  },

  
  pollEmailStatus: function pollEmailStatus(currentState, sessionToken, why) {
    log.debug("entering pollEmailStatus: " + why);
    if (why == "start") {
      
      
      
      this.pollStartDate = Date.now();
      if (!currentState.whenVerifiedDeferred) {
        currentState.whenVerifiedDeferred = Promise.defer();
        
        
        
        
        currentState.whenVerifiedDeferred.promise.then(null, err => {
          log.info("the wait for user verification was stopped: " + err);
        });
      }
    }

    this.checkEmailStatus(sessionToken)
      .then((response) => {
        log.debug("checkEmailStatus -> " + JSON.stringify(response));
        if (response && response.verified) {
          currentState.getUserAccountData()
            .then((data) => {
              data.verified = true;
              return currentState.setUserAccountData(data);
            })
            .then((data) => {
              
              if (currentState.whenVerifiedDeferred) {
                currentState.whenVerifiedDeferred.resolve(data);
                delete currentState.whenVerifiedDeferred;
              }
              
              this.notifyObservers(ON_FXA_UPDATE_NOTIFICATION, ONVERIFIED_NOTIFICATION);
            });
        } else {
          
          this.pollEmailStatusAgain(currentState, sessionToken);
        }
      }, error => {
        let timeoutMs = undefined;
        if (error && error.retryAfter) {
          
          timeoutMs = (error.retryAfter + 3) * 1000;
        }
        
        
        if (!error || !error.code || error.code != 401) {
          this.pollEmailStatusAgain(currentState, sessionToken, timeoutMs);
        }
      });
  },

  
  pollEmailStatusAgain: function (currentState, sessionToken, timeoutMs) {
    let ageMs = Date.now() - this.pollStartDate;
    if (ageMs >= this.POLL_SESSION) {
      if (currentState.whenVerifiedDeferred) {
        let error = new Error("User email verification timed out.")
        currentState.whenVerifiedDeferred.reject(error);
        delete currentState.whenVerifiedDeferred;
      }
      log.debug("polling session exceeded, giving up");
      return;
    }
    if (timeoutMs === undefined) {
      let currentMinute = Math.ceil(ageMs / 60000);
      timeoutMs = 1000 * (currentMinute <= 2 ? 5 : 15);
    }
    log.debug("polling with timeout = " + timeoutMs);
    this.currentTimer = setTimeout(() => {
      this.pollEmailStatus(currentState, sessionToken, "timer");
    }, timeoutMs);
  },

  _requireHttps: function() {
    let allowHttp = false;
    try {
      allowHttp = Services.prefs.getBoolPref("identity.fxaccounts.allowHttp");
    } catch(e) {
      
    }
    return allowHttp !== true;
  },

  
  getAccountsSignUpURI: function() {
    let url = Services.urlFormatter.formatURLPref("identity.fxaccounts.remote.signup.uri");
    if (this._requireHttps() && !/^https:/.test(url)) { 
      throw new Error("Firefox Accounts server must use HTTPS");
    }
    return url;
  },

  
  getAccountsSignInURI: function() {
    let url = Services.urlFormatter.formatURLPref("identity.fxaccounts.remote.signin.uri");
    if (this._requireHttps() && !/^https:/.test(url)) { 
      throw new Error("Firefox Accounts server must use HTTPS");
    }
    return url;
  },

  
  
  promiseAccountsForceSigninURI: function() {
    let url = Services.urlFormatter.formatURLPref("identity.fxaccounts.remote.force_auth.uri");
    if (this._requireHttps() && !/^https:/.test(url)) { 
      throw new Error("Firefox Accounts server must use HTTPS");
    }
    let currentState = this.currentAccountState;
    
    return this.getSignedInUser().then(accountData => {
      if (!accountData) {
        return null;
      }
      let newQueryPortion = url.indexOf("?") == -1 ? "?" : "&";
      newQueryPortion += "email=" + encodeURIComponent(accountData.email);
      return url + newQueryPortion;
    }).then(result => currentState.resolve(result));
  },

  
  
  
  
  promiseAccountsChangeProfileURI: function(settingToEdit = null) {
    let url = Services.urlFormatter.formatURLPref("identity.fxaccounts.settings.uri");

    if (settingToEdit) {
      url += (url.indexOf("?") == -1 ? "?" : "&") +
             "setting=" + encodeURIComponent(settingToEdit);
    }

    if (this._requireHttps() && !/^https:/.test(url)) { 
      throw new Error("Firefox Accounts server must use HTTPS");
    }
    let currentState = this.currentAccountState;
    
    return this.getSignedInUser().then(accountData => {
      if (!accountData) {
        return null;
      }
      let newQueryPortion = url.indexOf("?") == -1 ? "?" : "&";
      newQueryPortion += "email=" + encodeURIComponent(accountData.email);
      newQueryPortion += "&uid=" + encodeURIComponent(accountData.uid);
      return url + newQueryPortion;
    }).then(result => currentState.resolve(result));
  },

  

















  getOAuthToken: Task.async(function* (options = {}) {
    log.debug("getOAuthToken enter");

    if (!options.scope) {
      throw this._error(ERROR_INVALID_PARAMETER, "Missing 'scope' option");
    }

    let oAuthURL = Services.urlFormatter.formatURLPref("identity.fxaccounts.remote.oauth.uri");
    let client = options.client;

    if (!client) {
      try {
        client = new FxAccountsOAuthGrantClient({
          serverURL: oAuthURL,
          client_id: FX_OAUTH_CLIENT_ID
        });
      } catch (e) {
        throw this._error(ERROR_INVALID_PARAMETER, e);
      }
    }

    try {
      yield this._getVerifiedAccountOrReject();
      let assertion = yield this.getAssertion(oAuthURL);
      let result = yield client.getTokenFromAssertion(assertion, options.scope);
      return result.access_token;
    } catch (err) {
      throw this._errorToErrorClass(err);
    }
  }),

  _getVerifiedAccountOrReject: Task.async(function* () {
    let data = yield this.currentAccountState.getUserAccountData();
    if (!data) {
      
      throw this._error(ERROR_NO_ACCOUNT);
    }
    if (!this.isUserEmailVerified(data)) {
      
      throw this._error(ERROR_UNVERIFIED_ACCOUNT);
    }
  }),

  










  _errorToErrorClass: function (aError) {
    if (aError.errno) {
      let error = SERVER_ERRNO_TO_ERROR[aError.errno];
      return this._error(ERROR_TO_GENERAL_ERROR_CLASS[error] || ERROR_UNKNOWN, aError);
    } else if (aError.message &&
        (aError.message === "INVALID_PARAMETER" ||
        aError.message === "NO_ACCOUNT" ||
        aError.message === "UNVERIFIED_ACCOUNT")) {
      return aError;
    }
    return this._error(ERROR_UNKNOWN, aError);
  },

  _error: function(aError, aDetails) {
    log.error("FxA rejecting with error ${aError}, details: ${aDetails}", {aError, aDetails});
    let reason = new Error(aError);
    if (aDetails) {
      reason.details = aDetails;
    }
    return reason;
  },

  





















  getSignedInUserProfile: function () {
    let accountState = this.currentAccountState;
    return accountState.getProfile()
      .then((profileData) => {
        let profile = JSON.parse(JSON.stringify(profileData));
        return accountState.resolve(profile);
      },
      (error) => {
        log.error("Could not retrieve profile data", error);
        return accountState.reject(error);
      })
      .then(null, err => Promise.reject(this._errorToErrorClass(err)));
  },
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













function LoginManagerStorage(options) {
  
  this.jsonStorage = new JSONStorage(options);
}

LoginManagerStorage.prototype = {
  
  
  

  
  get _isLoggedIn() {
    return Services.logins.isLoggedIn;
  },

  
  
  
  _clearLoginMgrData: Task.async(function* () {
    try { 
      yield Services.logins.initializationPromise;
      if (!this._isLoggedIn) {
        return false;
      }
      let logins = Services.logins.findLogins({}, FXA_PWDMGR_HOST, null, FXA_PWDMGR_REALM);
      for (let login of logins) {
        Services.logins.removeLogin(login);
      }
      return true;
    } catch (ex) {
      log.error("Failed to clear login data: ${}", ex);
      return false;
    }
  }),

  set: Task.async(function* (contents) {
    if (!contents) {
      
      yield this.jsonStorage.set(contents);

      
      let cleared = yield this._clearLoginMgrData();
      if (!cleared) {
        
        
        log.info("not removing credentials from login manager - not logged in");
      }
      return;
    }

    
    
    
    let toWriteJSON = {version: contents.version};
    let accountDataJSON = toWriteJSON.accountData = {};
    let toWriteLoginMgr = {version: contents.version};
    let accountDataLoginMgr = toWriteLoginMgr.accountData = {};
    for (let [name, value] of Iterator(contents.accountData)) {
      if (FXA_PWDMGR_PLAINTEXT_FIELDS.indexOf(name) >= 0) {
        accountDataJSON[name] = value;
      } else {
        accountDataLoginMgr[name] = value;
      }
    }
    yield this.jsonStorage.set(toWriteJSON);

    try { 
      
      yield Services.logins.initializationPromise;
      
      
      if (!this._isLoggedIn) {
        log.info("not saving credentials to login manager - not logged in");
        return;
      }
      
      let loginInfo = new Components.Constructor(
         "@mozilla.org/login-manager/loginInfo;1", Ci.nsILoginInfo, "init");
      let login = new loginInfo(FXA_PWDMGR_HOST,
                                null, 
                                FXA_PWDMGR_REALM, 
                                contents.accountData.email, 
                                JSON.stringify(toWriteLoginMgr), 
                                "", 
                                "");

      let existingLogins = Services.logins.findLogins({}, FXA_PWDMGR_HOST, null,
                                                      FXA_PWDMGR_REALM);
      if (existingLogins.length) {
        Services.logins.modifyLogin(existingLogins[0], login);
      } else {
        Services.logins.addLogin(login);
      }
    } catch (ex) {
      log.error("Failed to save data to the login manager: ${}", ex);
    }
  }),

  get: Task.async(function* () {
    
    
    let data = yield this.jsonStorage.get();
    if (!data) {
      
      
      yield this._clearLoginMgrData();
      return null;
    }

    
    
    if (data.accountData.kA || data.accountData.kB || data.keyFetchToken) {
      
      
      
      
      
      
      
      
      
      
      if (!this._isLoggedIn) {
        
        log.info("account data needs migration to the login manager but the MP is locked.");
        let result = {
          version: data.version,
          accountData: {},
        };
        for (let fieldName of FXA_PWDMGR_PLAINTEXT_FIELDS) {
          result.accountData[fieldName] = data.accountData[fieldName];
        }
        return result;
      }
      
      log.info("account data is being migrated to the login manager.");
      yield this.set(data);
    }

    try { 
      
      yield Services.logins.initializationPromise;

      if (!this._isLoggedIn) {
        log.info("returning partial account data as the login manager is locked.");
        return data;
      }

      let logins = Services.logins.findLogins({}, FXA_PWDMGR_HOST, null, FXA_PWDMGR_REALM);
      if (logins.length == 0) {
        
        log.info("Can't find the rest of the credentials in the login manager");
        return data;
      }
      let login = logins[0];
      if (login.username == data.accountData.email) {
        let lmData = JSON.parse(login.password);
        if (lmData.version == data.version) {
          
          copyObjectProperties(lmData.accountData, data.accountData);
        } else {
          log.info("version field in the login manager doesn't match - ignoring it");
          yield this._clearLoginMgrData();
        }
      } else {
        log.info("username in the login manager doesn't match - ignoring it");
        yield this._clearLoginMgrData();
      }
    } catch (ex) {
      log.error("Failed to get data from the login manager: ${}", ex);
    }
    return data;
  }),

}


XPCOMUtils.defineLazyGetter(this, "fxAccounts", function() {
  let a = new FxAccounts();

  
  
  a.loadAndPoll();

  return a;
});
