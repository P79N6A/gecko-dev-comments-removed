






"use strict";

this.EXPORTED_SYMBOLS = ["MobileIdentityClient"];

const {classes: Cc, interfaces: Ci, utils: Cu} = Components;

Cu.import("resource://services-common/hawkclient.js");
Cu.import("resource://services-common/hawkrequest.js");
Cu.import("resource://services-common/utils.js");
Cu.import("resource://services-crypto/utils.js");
Cu.import("resource://gre/modules/MobileIdentityCommon.jsm");
Cu.import("resource://gre/modules/Promise.jsm");
Cu.import("resource://gre/modules/Services.jsm");

this.MobileIdentityClient = function(aServerUrl) {
  let serverUrl = aServerUrl || SERVER_URL;
  let forceHttps = true;
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
    return deriveHawkCredentials(aSessionToken, CREDENTIALS_DERIVATION_INFO,
                                 CREDENTIALS_DERIVATION_SIZE, true );
  },

  
















  _request: function(path, method, credentials, jsonPayload) {
    let deferred = Promise.defer();

    this.hawk.request(path, method, credentials, jsonPayload).then(
      (response) => {
        log.debug("MobileIdentityClient -> response.body " + response.body);
        try {
          let responseObj = JSON.parse(response.body);
          deferred.resolve(responseObj);
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
