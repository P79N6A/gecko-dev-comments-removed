



this.EXPORTED_SYMBOLS = ["FxAccountsClient"];

const {classes: Cc, interfaces: Ci, utils: Cu} = Components;

Cu.import("resource://gre/modules/Log.jsm");
Cu.import("resource://gre/modules/Promise.jsm");
Cu.import("resource://gre/modules/Services.jsm");
Cu.import("resource://services-common/utils.js");
Cu.import("resource://services-common/hawkclient.js");
Cu.import("resource://services-crypto/utils.js");
Cu.import("resource://gre/modules/FxAccountsCommon.js");
Cu.import("resource://gre/modules/Credentials.jsm");

const HOST = Services.prefs.getCharPref("identity.fxaccounts.auth.uri");

this.FxAccountsClient = function(host = HOST) {
  this.host = host;

  
  
  this.hawk = new HawkClient(host);
  this.hawk.observerPrefix = "FxA:hawk";

  
  
  this.backoffError = null;
};

this.FxAccountsClient.prototype = {

  








  get localtimeOffsetMsec() {
    return this.hawk.localtimeOffsetMsec;
  },

  





  now: function() {
    return this.hawk.now();
  },

  














  signUp: function(email, password) {
    return Credentials.setup(email, password).then((creds) => {
      let data = {
        email: creds.emailUTF8,
        authPW: CommonUtils.bytesAsHex(creds.authPW),
      };
      return this._request("/account/create", "POST", null, data);
    });
  },

  
























  signIn: function signIn(email, password, getKeys=false, retryOK=true) {
    return Credentials.setup(email, password).then((creds) => {
      let data = {
        authPW: CommonUtils.bytesAsHex(creds.authPW),
        email: creds.emailUTF8,
      };
      let keys = getKeys ? "?keys=true" : "";

      return this._request("/account/login" + keys, "POST", null, data).then(
        
        
        result => {
          result.email = data.email;
          result.unwrapBKey = CommonUtils.bytesAsHex(creds.unwrapBKey);

          return result;
        },
        error => {
          log.debug("signIn error: " + JSON.stringify(error));
          
          
          
          
          
          
          
          
          
          
          
          if (ERRNO_INCORRECT_EMAIL_CASE === error.errno && retryOK) {
            if (!error.email) {
              log.error("Server returned errno 120 but did not provide email");
              throw error;
            }
            return this.signIn(error.email, password, getKeys, false);
          }
          throw error;
        }
      );
    });
  },

  






  signOut: function (sessionTokenHex) {
    return this._request("/session/destroy", "POST",
      this._deriveHawkCredentials(sessionTokenHex, "sessionToken"));
  },

  






  recoveryEmailStatus: function (sessionTokenHex) {
    return this._request("/recovery_email/status", "GET",
      this._deriveHawkCredentials(sessionTokenHex, "sessionToken"));
  },

  






  resendVerificationEmail: function(sessionTokenHex) {
    return this._request("/recovery_email/resend_code", "POST",
      this._deriveHawkCredentials(sessionTokenHex, "sessionToken"));
  },

  












  accountKeys: function (keyFetchTokenHex) {
    let creds = this._deriveHawkCredentials(keyFetchTokenHex, "keyFetchToken");
    let keyRequestKey = creds.extra.slice(0, 32);
    let morecreds = CryptoUtils.hkdf(keyRequestKey, undefined,
                                     Credentials.keyWord("account/keys"), 3 * 32);
    let respHMACKey = morecreds.slice(0, 32);
    let respXORKey = morecreds.slice(32, 96);

    return this._request("/account/keys", "GET", creds).then(resp => {
      if (!resp.bundle) {
        throw new Error("failed to retrieve keys");
      }

      let bundle = CommonUtils.hexToBytes(resp.bundle);
      let mac = bundle.slice(-32);

      let hasher = CryptoUtils.makeHMACHasher(Ci.nsICryptoHMAC.SHA256,
        CryptoUtils.makeHMACKey(respHMACKey));

      let bundleMAC = CryptoUtils.digestBytes(bundle.slice(0, -32), hasher);
      if (mac !== bundleMAC) {
        throw new Error("error unbundling encryption keys");
      }

      let keyAWrapB = CryptoUtils.xor(respXORKey, bundle.slice(0, 64));

      return {
        kA: keyAWrapB.slice(0, 32),
        wrapKB: keyAWrapB.slice(32)
      };
    });
  },

  















  signCertificate: function (sessionTokenHex, serializedPublicKey, lifetime) {
    let creds = this._deriveHawkCredentials(sessionTokenHex, "sessionToken");

    let body = { publicKey: serializedPublicKey,
                 duration: lifetime };
    return Promise.resolve()
      .then(_ => this._request("/certificate/sign", "POST", creds, body))
      .then(resp => resp.cert,
            err => {
              log.error("HAWK.signCertificate error: " + JSON.stringify(err));
              throw err;
            });
  },

  








  accountExists: function (email) {
    return this.signIn(email, "").then(
      (cantHappen) => {
        throw new Error("How did I sign in with an empty password?");
      },
      (expectedError) => {
        switch (expectedError.errno) {
          case ERRNO_ACCOUNT_DOES_NOT_EXIST:
            return false;
            break;
          case ERRNO_INCORRECT_PASSWORD:
            return true;
            break;
          default:
            
            throw expectedError;
            break;
        }
      }
    );
  },

  





  accountStatus: function(uid) {
    return this._request("/account/status?uid="+uid, "GET").then(
      (result) => {
        return result.exists;
      },
      (error) => {
        log.error("accountStatus failed with: " + error);
        return Promise.reject(error);
      }
    );
  },

  



















  _deriveHawkCredentials: function (tokenHex, context, size) {
    let token = CommonUtils.hexToBytes(tokenHex);
    let out = CryptoUtils.hkdf(token, undefined, Credentials.keyWord(context), size || 3 * 32);

    return {
      algorithm: "sha256",
      key: out.slice(32, 64),
      extra: out.slice(64),
      id: CommonUtils.bytesAsHex(out.slice(0, 32))
    };
  },

  _clearBackoff: function() {
      this.backoffError = null;
  },

  






















  _request: function hawkRequest(path, method, credentials, jsonPayload) {
    let deferred = Promise.defer();

    
    if (this.backoffError) {
      log.debug("Received new request during backoff, re-rejecting.");
      deferred.reject(this.backoffError);
      return deferred.promise;
    }

    this.hawk.request(path, method, credentials, jsonPayload).then(
      (responseText) => {
        try {
          let response = JSON.parse(responseText);
          deferred.resolve(response);
        } catch (err) {
          log.error("json parse error on response: " + responseText);
          deferred.reject({error: err});
        }
      },

      (error) => {
        log.error("error " + method + "ing " + path + ": " + JSON.stringify(error));
        if (error.retryAfter) {
          log.debug("Received backoff response; caching error as flag.");
          this.backoffError = error;
          
          CommonUtils.namedTimer(
            this._clearBackoff,
            error.retryAfter * 1000,
            this,
            "fxaBackoffTimer"
           );
	}
        deferred.reject(error);
      }
    );

    return deferred.promise;
  },
};

