



this.EXPORTED_SYMBOLS = ["FxAccountsClient"];

const {classes: Cc, interfaces: Ci, utils: Cu} = Components;

Cu.import("resource://gre/modules/Log.jsm");
Cu.import("resource://gre/modules/Promise.jsm");
Cu.import("resource://gre/modules/Services.jsm");
Cu.import("resource://services-common/utils.js");
Cu.import("resource://services-common/hawk.js");
Cu.import("resource://services-crypto/utils.js");
Cu.import("resource://gre/modules/FxAccountsCommon.js");


let _host = "https://api-accounts.dev.lcip.org/v1";
try {
  _host = Services.prefs.getCharPref("identity.fxaccounts.auth.uri");
} catch(keepDefault) {}

const HOST = _host;
const PROTOCOL_VERSION = "identity.mozilla.com/picl/v1/";

function KW(context) {
  
  
  
  
  
  
  return PROTOCOL_VERSION + context;
}

function stringToHex(str) {
  let encoder = new TextEncoder("utf-8");
  let bytes = encoder.encode(str);
  return bytesToHex(bytes);
}


function bytesToHex(bytes) {
  let hex = [];
  for (let i = 0; i < bytes.length; i++) {
    hex.push((bytes[i] >>> 4).toString(16));
    hex.push((bytes[i] & 0xF).toString(16));
  }
  return hex.join("");
}

this.FxAccountsClient = function(host = HOST) {
  this.host = host;

  
  
  this.hawk = new HawkClient(host);
};

this.FxAccountsClient.prototype = {

  








  get localtimeOffsetMsec() {
    return this.hawk.localtimeOffsetMsec;
  },

  





  now: function() {
    return this.hawk.now();
  },

  













  signUp: function (email, password) {
    let uid;
    let hexEmail = stringToHex(email);
    let uidPromise = this._request("/raw_password/account/create", "POST", null,
                          {email: hexEmail, password: password});

    return uidPromise.then((result) => {
      uid = result.uid;
      return this.signIn(email, password)
        .then(function(result) {
          result.uid = uid;
          return result;
        });
    });
  },

  














  signIn: function signIn(email, password) {
    let hexEmail = stringToHex(email);
    return this._request("/raw_password/session/create", "POST", null,
                         {email: hexEmail, password: password});
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
                                     KW("account/keys"), 3 * 32);
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
    let hexEmail = stringToHex(email);
    return this._request("/auth/start", "POST", null, { email: hexEmail })
      .then(
        
        (result) => true,
        (err) => {
          log.error("accountExists: error: " + JSON.stringify(err));
          
          if (err.errno === 102) {
            log.debug("returning false for errno 102");
            return false;
          }
          
          throw err;
        }
      );
  },

  



















  _deriveHawkCredentials: function (tokenHex, context, size) {
    let token = CommonUtils.hexToBytes(tokenHex);
    let out = CryptoUtils.hkdf(token, undefined, KW(context), size || 3 * 32);

    return {
      algorithm: "sha256",
      key: out.slice(32, 64),
      extra: out.slice(64),
      id: CommonUtils.bytesAsHex(out.slice(0, 32))
    };
  },

  






















  _request: function hawkRequest(path, method, credentials, jsonPayload) {
    let deferred = Promise.defer();

    this.hawk.request(path, method, credentials, jsonPayload).then(
      (responseText) => {
        try {
          let response = JSON.parse(responseText);
          deferred.resolve(response);
        } catch (err) {
          deferred.reject({error: err});
        }
      },

      (error) => {
        deferred.reject(error);
      }
    );

    return deferred.promise;
  },
};

