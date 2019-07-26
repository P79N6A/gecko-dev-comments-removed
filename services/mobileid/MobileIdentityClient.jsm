






"use strict";

this.EXPORTED_SYMBOLS = ["MobileIdentityClient"];

const {classes: Cc, interfaces: Ci, utils: Cu} = Components;

Cu.import("resource://services-common/hawkclient.js");
Cu.import("resource://services-common/utils.js");
Cu.import("resource://services-crypto/utils.js");
Cu.import("resource://gre/modules/MobileIdentityCommon.jsm");
Cu.import("resource://gre/modules/Promise.jsm");
Cu.import("resource://gre/modules/Services.jsm");

this.MobileIdentityClient = function(aServerUrl) {
  let serverUrl = aServerUrl || SERVER_URL;
  let forceHttps = false;
  try {
    
    forceHttps = Services.prefs.getBoolPref(PREF_FORCE_HTTPS);
  } catch(e) {
    log.warn("Getting force HTTPS pref failed. If this was not intentional " +
             "check that " + PREF_FORCE_HTTPS + " is defined");
  }

  log.debug("Force HTTPS " + forceHttps);

  if (forceHttps && !/^https/.exec(serverUrl.toLowerCase())) {
    throw new Error(ERROR_INTERNAL_HTTP_NOT_ALLOWED);
  }

  this.hawk = new HawkClient(SERVER_URL);
  this.hawk.observerPrefix = "MobileId:hawk";
};

this.MobileIdentityClient.prototype = {

  discover: function(aMsisdn, aMcc, aMnc, aRoaming) {
    return this._request(DISCOVER, "POST", null, {
      msisdn: aMsisdn || undefined,
      mcc: aMcc,
      mnc: aMnc,
      roaming: aRoaming
    });
  },

  register: function() {
    return this._request(REGISTER, "POST", null, {});
  },

  smsMtVerify: function(aSessionToken, aMsisdn, aWantShortCode = false) {
    let credentials = this._deriveHawkCredentials(aSessionToken);
    return this._request(SMS_MT_VERIFY, "POST", credentials, {
      msisdn: aMsisdn,
      shortVerificationCode: aWantShortCode
    });
  },

  verifyCode: function(aSessionToken, aVerificationCode) {
    log.debug("verificationCode " + aVerificationCode);
    let credentials = this._deriveHawkCredentials(aSessionToken);
    return this._request(SMS_VERIFY_CODE, "POST", credentials, {
      code: aVerificationCode
    });
  },

  sign: function(aSessionToken, aDuration, aPublicKey) {
    let credentials = this._deriveHawkCredentials(aSessionToken);
    return this._request(SIGN, "POST", credentials, {
      duration: aDuration,
      publicKey: aPublicKey
    });
  },

  unregister: function(aSessionToken) {
    let credentials = this._deriveHawkCredentials(aSessionToken);
    return this._request(UNREGISTER, "POST", credentials, {});
  },

  



















  _deriveHawkCredentials: function(aSessionToken) {
    let token = CommonUtils.hexToBytes(aSessionToken);
    let out = CryptoUtils.hkdf(token, undefined,
                               CREDENTIALS_DERIVATION_INFO,
                               CREDENTIALS_DERIVATION_SIZE);
    return {
      algorithm: "sha256",
      key: CommonUtils.bytesAsHex(out.slice(32, 64)),
      id: CommonUtils.bytesAsHex(out.slice(0, 32))
    };
  },

  
















  _request: function(path, method, credentials, jsonPayload) {
    let deferred = Promise.defer();

    this.hawk.request(path, method, credentials, jsonPayload).then(
      (responseText) => {
        log.debug("MobileIdentityClient -> responseText " + responseText);
        try {
          let response = JSON.parse(responseText);
          deferred.resolve(response);
        } catch (err) {
          deferred.reject({error: err});
        }
      },

      (error) => {
        log.error("MobileIdentityClient -> Error ${}", error);
        deferred.reject(SERVER_ERRNO_TO_ERROR[error.errno] || ERROR_UNKNOWN);
      }
    );

    return deferred.promise;
  },

};
