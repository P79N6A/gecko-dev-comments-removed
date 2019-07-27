


"use strict";

this.EXPORTED_SYMBOLS = ["fxAccounts", "FxAccounts"];

const {classes: Cc, interfaces: Ci, utils: Cu} = Components;

Cu.import("resource://gre/modules/Log.jsm");
Cu.import("resource://gre/modules/Promise.jsm");
Cu.import("resource://services-common/utils.js");
Cu.import("resource://services-crypto/utils.js");
Cu.import("resource://gre/modules/Services.jsm");
Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://gre/modules/Timer.jsm");
Cu.import("resource://gre/modules/Task.jsm");
Cu.import("resource://gre/modules/FxAccountsStorage.jsm");
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
  "promiseAccountsManageURI",
  "removeCachedOAuthToken",
  "resendVerificationEmail",
  "setSignedInUser",
  "signOut",
  "whenVerified"
];



















let AccountState = function(fxaInternal, storageManager) {
  this.fxaInternal = fxaInternal;
  this.storageManager = storageManager;
  this.promiseInitialized = this.storageManager.getAccountData().then(data => {
    this.oauthTokens = data && data.oauthTokens ? data.oauthTokens : {};
  }).catch(err => {
    log.error("Failed to initialize the storage manager", err);
    
  });
};

AccountState.prototype = {
  cert: null,
  keyPair: null,
  oauthTokens: null,
  whenVerifiedDeferred: null,
  whenKeysReadyDeferred: null,

  get isCurrent() this.fxaInternal && this.fxaInternal.currentAccountState === this,

  abort() {
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
    this.oauthTokens = null;
    this.fxaInternal = null;
    
    
    if (!this.storageManager) {
      return Promise.resolve();
    }
    let storageManager = this.storageManager;
    this.storageManager = null;
    return storageManager.finalize();
  },

  
  signOut() {
    this.cert = null;
    this.keyPair = null;
    this.oauthTokens = null;
    let storageManager = this.storageManager;
    this.storageManager = null;
    return storageManager.deleteAccountData().then(() => {
      return storageManager.finalize();
    });
  },

  getUserAccountData() {
    if (!this.isCurrent) {
      return Promise.reject(new Error("Another user has signed in"));
    }
    return this.storageManager.getAccountData().then(result => {
      return this.resolve(result);
    });
  },

  updateUserAccountData(updatedFields) {
    if (!this.isCurrent) {
      return Promise.reject(new Error("Another user has signed in"));
    }
    return this.storageManager.updateAccountData(updatedFields);
  },

  getCertificate: function(data, keyPair, mustBeValidUntil) {
    
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

  resolve: function(result) {
    if (!this.isCurrent) {
      log.info("An accountState promise was resolved, but was actually rejected" +
               " due to a different user being signed in. Originally resolved" +
               " with", result);
      return Promise.reject(new Error("A different user signed in"));
    }
    return Promise.resolve(result);
  },

  reject: function(error) {
    
    
    
    
    if (!this.isCurrent) {
      log.info("An accountState promise was rejected, but we are ignoring that" +
               "reason and rejecting it due to a different user being signed in." +
               "Originally rejected with", error);
      return Promise.reject(new Error("A different user signed in"));
    }
    return Promise.reject(error);
  },

  
  
  
  
  
  

  
  _cachePreamble() {
    if (!this.isCurrent) {
      throw new Error("Another user has signed in");
    }
  },

  
  
  
  setCachedToken(scopeArray, tokenData) {
    this._cachePreamble();
    if (!tokenData.token) {
      throw new Error("No token");
    }
    let key = getScopeKey(scopeArray);
    this.oauthTokens[key] = tokenData;
    
    this._persistCachedTokens();
  },

  
  getCachedToken(scopeArray) {
    this._cachePreamble();
    let key = getScopeKey(scopeArray);
    let result = this.oauthTokens[key];
    if (result) {
      
      
      log.trace("getCachedToken returning cached token");
      return result;
    }
    return null;
  },

  
  
  removeCachedToken(token) {
    this._cachePreamble();
    let data = this.oauthTokens;
    for (let [key, tokenValue] in Iterator(data)) {
      if (tokenValue.token == token) {
        delete data[key];
        
        this._persistCachedTokens();
        return tokenValue;
      }
    }
    return null;
  },

  
  
  
  _persistCachedTokens() {
    this._cachePreamble();
    return this.updateUserAccountData({ oauthTokens: this.oauthTokens }).catch(err => {
      log.error("Failed to update cached tokens", err);
    });
  },
}


function getScopeKey(scopeArray) {
  let normalizedScopes = scopeArray.map(item => item.toLowerCase());
  return normalizedScopes.sort().join("|");
}


















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

  
  internal.initialize();

  return Object.freeze(external);
}




function FxAccountsInternal() {
  
  this.POLL_SESSION = POLL_SESSION;

  
  
}




FxAccountsInternal.prototype = {
  
  VERIFICATION_POLL_TIMEOUT_INITIAL: 5000, 
  
  VERIFICATION_POLL_TIMEOUT_SUBSEQUENT: 15000, 

  _fxAccountsClient: null,

  
  
  initialize() {
    this.currentTimer = null;
    this.currentAccountState = this.newAccountState();
  },

  get fxAccountsClient() {
    if (!this._fxAccountsClient) {
      this._fxAccountsClient = new FxAccountsClient();
    }
    return this._fxAccountsClient;
  },

  
  _profile: null,
  get profile() {
    if (!this._profile) {
      let profileServerUrl = Services.urlFormatter.formatURLPref("identity.fxaccounts.remote.profile.uri");
      this._profile = new FxAccountsProfile({
        fxa: this,
        profileServerUrl: profileServerUrl,
      });
    }
    return this._profile;
  },

  
  newAccountState(credentials) {
    let storage = new FxAccountsStorageManager();
    storage.initialize(credentials);
    return new AccountState(this, storage);
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
    return this.abortExistingFlow().then(() => {
      let currentAccountState = this.currentAccountState = this.newAccountState(
        Cu.cloneInto(credentials, {}) 
      );
      
      
      
      
      return currentAccountState.promiseInitialized.then(() => {
        this.notifyObservers(ONLOGIN_NOTIFICATION);
        if (!this.isUserEmailVerified(credentials)) {
          this.startVerifiedCheck(credentials);
        }
      }).then(() => {
        return currentAccountState.resolve();
      });
    })
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
    if (this._profile) {
      this._profile.tearDown();
      this._profile = null;
    }
    
    
    return this.currentAccountState.abort();
  },

  accountStatus: function accountStatus() {
    return this.currentAccountState.getUserAccountData().then(data => {
      if (!data) {
        return false;
      }
      return this.fxAccountsClient.accountStatus(data.uid);
    });
  },

  _destroyOAuthToken: function(tokenData) {
    let client = new FxAccountsOAuthGrantClient({
      serverURL: tokenData.server,
      client_id: FX_OAUTH_CLIENT_ID
    });
    return client.destroyToken(tokenData.token)
  },

  _destroyAllOAuthTokens: function(tokenInfos) {
    
    let promises = [];
    for (let [key, tokenInfo] in Iterator(tokenInfos || {})) {
      promises.push(this._destroyOAuthToken(tokenInfo));
    }
    return Promise.all(promises);
  },

  signOut: function signOut(localOnly) {
    let currentState = this.currentAccountState;
    let sessionToken;
    let tokensToRevoke;
    return currentState.getUserAccountData().then(data => {
      
      sessionToken = data && data.sessionToken;
      tokensToRevoke = data && data.oauthTokens;
      return this._signOutLocal();
    }).then(() => {
      
      
      if (!localOnly) {
        
        
        Promise.resolve().then(() => {
          
          
          
          return this._signOutServer(sessionToken);
        }).catch(err => {
          log.error("Error during remote sign out of Firefox Accounts", err);
        }).then(() => {
          return this._destroyAllOAuthTokens(tokensToRevoke);
        }).catch(err => {
          log.error("Error during destruction of oauth tokens during signout", err);
        }).then(() => {
          
          this.notifyObservers("testhelper-fxa-signout-complete");
        });
      }
    }).then(() => {
      this.notifyObservers(ONLOGOUT_NOTIFICATION);
    });
  },

  



  _signOutLocal: function signOutLocal() {
    let currentAccountState = this.currentAccountState;
    return currentAccountState.signOut().then(() => {
      
      return this.abortExistingFlow();
    }).then(() => {
      this.currentAccountState = this.newAccountState();
      return this.currentAccountState.promiseInitialized;
    });
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
      let updateData = {
        kA: CommonUtils.bytesAsHex(kA),
        kB: CommonUtils.bytesAsHex(kB_hex),
        keyFetchToken: null, 
        unwrapBKey: null,
      }

      log.debug("Keys Obtained: kA=" + !!updateData.kA + ", kB=" + !!updateData.kB);
      if (logPII) {
        log.debug("Keys Obtained: kA=" + updateData.kA + ", kB=" + updateData.kB);
      }

      yield currentState.updateUserAccountData(updateData);
      
      
      
      this.notifyObservers(ONVERIFIED_NOTIFICATION);
      return currentState.getUserAccountData();
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
      if (this.currentTimer) {
        log.debug("pollEmailStatus starting while existing timer is running");
        clearTimeout(this.currentTimer);
        this.currentTimer = null;
      }

      
      
      
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
          currentState.updateUserAccountData({ verified: true })
            .then(() => {
              return currentState.getUserAccountData();
            })
            .then(data => {
              
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
      timeoutMs = currentMinute <= 2 ? this.VERIFICATION_POLL_TIMEOUT_INITIAL
                                     : this.VERIFICATION_POLL_TIMEOUT_SUBSEQUENT;
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

  
  
  
  
  promiseAccountsChangeProfileURI: function(entrypoint, settingToEdit = null) {
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
      if (entrypoint) {
        newQueryPortion += "&entrypoint=" + encodeURIComponent(entrypoint);
      }
      return url + newQueryPortion;
    }).then(result => currentState.resolve(result));
  },

  
  
  promiseAccountsManageURI: function(entrypoint) {
    let url = Services.urlFormatter.formatURLPref("identity.fxaccounts.settings.uri");
    if (this._requireHttps() && !/^https:/.test(url)) { 
      throw new Error("Firefox Accounts server must use HTTPS");
    }
    let currentState = this.currentAccountState;
    
    
    
    return this.getSignedInUser().then(accountData => {
      if (!accountData) {
        return null;
      }
      let newQueryPortion = url.indexOf("?") == -1 ? "?" : "&";
      newQueryPortion += "uid=" + encodeURIComponent(accountData.uid) +
                         "&email=" + encodeURIComponent(accountData.email);
      if (entrypoint) {
        newQueryPortion += "&entrypoint=" + encodeURIComponent(entrypoint);
      }
      return url + newQueryPortion;
    }).then(result => currentState.resolve(result));
  },

  



















  getOAuthToken: Task.async(function* (options = {}) {
    log.debug("getOAuthToken enter");
    let scope = options.scope;
    if (typeof scope === "string") {
      scope = [scope];
    }

    if (!scope || !scope.length) {
      throw this._error(ERROR_INVALID_PARAMETER, "Missing or invalid 'scope' option");
    }

    yield this._getVerifiedAccountOrReject();

    
    let currentState = this.currentAccountState;
    let cached = currentState.getCachedToken(scope);
    if (cached) {
      log.debug("getOAuthToken returning a cached token");
      return cached.token;
    }

    
    let scopeString = scope.join(" ");
    let client = options.client;

    if (!client) {
      try {
        let defaultURL = Services.urlFormatter.formatURLPref("identity.fxaccounts.remote.oauth.uri");
        client = new FxAccountsOAuthGrantClient({
          serverURL: defaultURL,
          client_id: FX_OAUTH_CLIENT_ID
        });
      } catch (e) {
        throw this._error(ERROR_INVALID_PARAMETER, e);
      }
    }
    let oAuthURL = client.serverURL.href;

    try {
      log.debug("getOAuthToken fetching new token from", oAuthURL);
      let assertion = yield this.getAssertion(oAuthURL);
      let result = yield client.getTokenFromAssertion(assertion, scopeString);
      let token = result.access_token;
      
      if (token) {
        let entry = {token: token, server: oAuthURL};
        
        
        
        
        let cached = currentState.getCachedToken(scope);
        if (cached) {
          log.debug("Detected a race for this token - revoking the new one.");
          this._destroyOAuthToken(entry);
          return cached.token;
        }
        currentState.setCachedToken(scope, entry);
      }
      return token;
    } catch (err) {
      throw this._errorToErrorClass(err);
    }
  }),

  











   removeCachedOAuthToken: Task.async(function* (options) {
    if (!options.token || typeof options.token !== "string") {
      throw this._error(ERROR_INVALID_PARAMETER, "Missing or invalid 'token' option");
    }
    let currentState = this.currentAccountState;
    let existing = currentState.removeCachedToken(options.token);
    if (existing) {
      
      this._destroyOAuthToken(existing).catch(err => {
        log.warn("FxA failed to revoke a cached token", err);
      });
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
    let currentState = this.currentAccountState;
    return this.profile.getProfile().then(
      profileData => {
        let profile = Cu.cloneInto(profileData, {});
        return currentState.resolve(profile);
      },
      error => {
        log.error("Could not retrieve profile data", error);
        return currentState.reject(error);
      }
    ).catch(err => Promise.reject(this._errorToErrorClass(err)));
  },
};



XPCOMUtils.defineLazyGetter(this, "fxAccounts", function() {
  let a = new FxAccounts();

  
  
  a.loadAndPoll();

  return a;
});
