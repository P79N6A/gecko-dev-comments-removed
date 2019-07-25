




































const Cc = Components.classes;
const Ci = Components.interfaces;
const Cr = Components.results;
const Cu = Components.utils;

Cu.import("resource://services-sync/log4moz.js");
Cu.import("resource://services-sync/auth.js");
Cu.import("resource://services-sync/resource.js");
Cu.import("resource://services-sync/constants.js");
Cu.import("resource://services-sync/util.js");

const EXPORTED_SYMBOLS = ["JPAKEClient"];

const JPAKE_SIGNERID_SENDER   = "sender";
const JPAKE_SIGNERID_RECEIVER = "receiver";
const JPAKE_LENGTH_SECRET     = 8;
const JPAKE_LENGTH_CLIENTID   = 256;
const JPAKE_VERIFY_VALUE      = "0123456789ABCDEF";





































































function JPAKEClient(observer) {
  this.observer = observer;

  this._log = Log4Moz.repository.getLogger("Service.JPAKEClient");
  this._log.level = Log4Moz.Level[Svc.Prefs.get(
    "log.logger.service.jpakeclient", "Debug")];

  this._serverUrl = Svc.Prefs.get("jpake.serverURL");
  this._pollInterval = Svc.Prefs.get("jpake.pollInterval");
  this._maxTries = Svc.Prefs.get("jpake.maxTries");
  if (this._serverUrl.slice(-1) != "/")
    this._serverUrl += "/";

  this._jpake = Cc["@mozilla.org/services-crypto/sync-jpake;1"]
                  .createInstance(Ci.nsISyncJPAKE);
  this._auth = new NoOpAuthenticator();

  this._setClientID();
}
JPAKEClient.prototype = {

  _chain: Utils.asyncChain,

  



  receiveNoPIN: function receiveNoPIN() {
    this._my_signerid = JPAKE_SIGNERID_RECEIVER;
    this._their_signerid = JPAKE_SIGNERID_SENDER;

    this._secret = this._createSecret();

    
    
    this._maxTries = Svc.Prefs.get("jpake.firstMsgMaxTries");
    this._chain(this._getChannel,
                this._computeStepOne,
                this._putStep,
                this._getStep,
                function(callback) {
                  
                  this._maxTries = Svc.Prefs.get("jpake.maxTries");
                  callback();
                },
                this._computeStepTwo,
                this._putStep,
                this._getStep,
                this._computeFinal,
                this._computeKeyVerification,
                this._putStep,
                this._getStep,
                this._decryptData,
                this._complete)();
  },

  sendWithPIN: function sendWithPIN(pin, obj) {
    this._my_signerid = JPAKE_SIGNERID_SENDER;
    this._their_signerid = JPAKE_SIGNERID_RECEIVER;

    this._channel = pin.slice(JPAKE_LENGTH_SECRET);
    this._channelUrl = this._serverUrl + this._channel;
    this._secret = pin.slice(0, JPAKE_LENGTH_SECRET);
    this._data = JSON.stringify(obj);

    this._chain(this._computeStepOne,
                this._getStep,
                this._putStep,
                this._computeStepTwo,
                this._getStep,
                this._putStep,
                this._computeFinal,
                this._getStep,
                this._encryptData,
                this._putStep,
                this._complete)();
  },

  abort: function abort(error) {
    this._log.debug("Aborting...");
    this._finished = true;
    let self = this;
    if (error == JPAKE_ERROR_CHANNEL
        || error == JPAKE_ERROR_NETWORK
        || error == JPAKE_ERROR_NODATA) {
      Utils.delay(function() { this.observer.onAbort(error); }, 0,
                  this, "_timer_onAbort");
    } else {
      this._reportFailure(error, function() { self.observer.onAbort(error); });
    }
  },

  



  _setClientID: function _setClientID() {
    let rng = Cc["@mozilla.org/security/random-generator;1"]
                .createInstance(Ci.nsIRandomGenerator);
    let bytes = rng.generateRandomBytes(JPAKE_LENGTH_CLIENTID / 2);
    this._clientID = [("0" + byte.toString(16)).slice(-2)
                      for each (byte in bytes)].join("");
  },

  _createSecret: function _createSecret() {
    
    const key = "23456789abcdefghijkmnpqrstuvwxyz";
    let rng = Cc["@mozilla.org/security/random-generator;1"]
                .createInstance(Ci.nsIRandomGenerator);
    let bytes = rng.generateRandomBytes(JPAKE_LENGTH_SECRET);
    return [key[Math.floor(byte * key.length / 256)]
            for each (byte in bytes)].join("");
  },

  



  _getChannel: function _getChannel(callback) {
    this._log.trace("Requesting channel.");
    let resource = new AsyncResource(this._serverUrl + "new_channel");
    resource.authenticator = this._auth;
    resource.setHeader("X-KeyExchange-Id", this._clientID);
    resource.get(Utils.bind2(this, function handleChannel(error, response) {
      if (this._finished)
        return;

      if (error) {
        this._log.error("Error acquiring channel ID. " + error);
        this.abort(JPAKE_ERROR_CHANNEL);
        return;
      }
      if (response.status != 200) {
        this._log.error("Error acquiring channel ID. Server responded with HTTP "
                        + response.status);
        this.abort(JPAKE_ERROR_CHANNEL);
        return;
      }

      let channel;
      try {
        this._channel = response.obj;
      } catch (ex) {
        this._log.error("Server responded with invalid JSON.");
        this.abort(JPAKE_ERROR_CHANNEL);
        return;
      }
      this._log.debug("Using channel " + this._channel);
      this._channelUrl = this._serverUrl + this._channel;

      
      let pin = this._secret + this._channel;
      Utils.delay(function() { this.observer.displayPIN(pin); }, 0,
                  this, "_timer_displayPIN");
      callback();
    }));
  },

  
  _putStep: function _putStep(callback) {
    this._log.trace("Uploading message " + this._outgoing.type);
    let resource = new AsyncResource(this._channelUrl);
    resource.authenticator = this._auth;
    resource.setHeader("X-KeyExchange-Id", this._clientID);
    resource.put(this._outgoing, Utils.bind2(this, function (error, response) {
      if (this._finished)
        return;

      if (error) {
        this._log.error("Error uploading data. " + error);
        this.abort(JPAKE_ERROR_NETWORK);
        return;
      }
      if (response.status != 200) {
        this._log.error("Could not upload data. Server responded with HTTP "
                        + response.status);
        this.abort(JPAKE_ERROR_SERVER);
        return;
      }
      
      
      this._etag = response.headers["etag"];
      Utils.delay(function () { callback(); }, this._pollInterval * 2, this,
                  "_pollTimer");
    }));
  },

  
  _pollTries: 0,
  _getStep: function _getStep(callback) {
    this._log.trace("Retrieving next message.");
    let resource = new AsyncResource(this._channelUrl);
    resource.authenticator = this._auth;
    resource.setHeader("X-KeyExchange-Id", this._clientID);
    if (this._etag)
      resource.setHeader("If-None-Match", this._etag);

    resource.get(Utils.bind2(this, function (error, response) {
      if (this._finished)
        return;

      if (error) {
        this._log.error("Error fetching data. " + error);
        this.abort(JPAKE_ERROR_NETWORK);
        return;
      }

      if (response.status == 304) {
        this._log.trace("Channel hasn't been updated yet. Will try again later.");
        if (this._pollTries >= this._maxTries) {
          this._log.error("Tried for " + this._pollTries + " times, aborting.");
          this.abort(JPAKE_ERROR_TIMEOUT);
          return;
        }
        this._pollTries += 1;
        Utils.delay(function() { this._getStep(callback); },
                    this._pollInterval, this, "_pollTimer");
        return;
      }
      this._pollTries = 0;

      if (response.status == 404) {
        this._log.error("No data found in the channel.");
        this.abort(JPAKE_ERROR_NODATA);
        return;
      }
      if (response.status != 200) {
        this._log.error("Could not retrieve data. Server responded with HTTP "
                        + response.status);
        this.abort(JPAKE_ERROR_SERVER);
        return;
      }

      try {
        this._incoming = response.obj;
      } catch (ex) {
        this._log.error("Server responded with invalid JSON.");
        this.abort(JPAKE_ERROR_INVALID);
        return;
      }
      this._log.trace("Fetched message " + this._incoming.type);
      callback();
    }));
  },

  _reportFailure: function _reportFailure(reason, callback) {
    this._log.debug("Reporting failure to server.");
    let resource = new AsyncResource(this._serverUrl + "report");
    resource.authenticator = this._auth;
    resource.setHeader("X-KeyExchange-Id", this._clientID);
    resource.setHeader("X-KeyExchange-Cid", this._channel);
    resource.setHeader("X-KeyExchange-Log", reason);
    resource.post("", Utils.bind2(this, function (error, response) {
      if (error)
        this._log.warn("Report failed: " + error);
      else if (response.status != 200)
        this._log.warn("Report failed. Server responded with HTTP "
                       + response.status);

      
      callback();
    }));
  },

  _computeStepOne: function _computeStepOne(callback) {
    this._log.trace("Computing round 1.");
    let gx1 = {};
    let gv1 = {};
    let r1 = {};
    let gx2 = {};
    let gv2 = {};
    let r2 = {};
    try {
      this._jpake.round1(this._my_signerid, gx1, gv1, r1, gx2, gv2, r2);
    } catch (ex) {
      this._log.error("JPAKE round 1 threw: " + ex);
      this.abort(JPAKE_ERROR_INTERNAL);
      return;
    }
    let one = {gx1: gx1.value,
               gx2: gx2.value,
               zkp_x1: {gr: gv1.value, b: r1.value, id: this._my_signerid},
               zkp_x2: {gr: gv2.value, b: r2.value, id: this._my_signerid}};
    this._outgoing = {type: this._my_signerid + "1", payload: one};
    this._log.trace("Generated message " + this._outgoing.type);
    callback();
  },

  _computeStepTwo: function _computeStepTwo(callback) {
    this._log.trace("Computing round 2.");
    if (this._incoming.type != this._their_signerid + "1") {
      this._log.error("Invalid round 1 message: "
                      + JSON.stringify(this._incoming));
      this.abort(JPAKE_ERROR_WRONGMESSAGE);
      return;
    }

    let step1 = this._incoming.payload;
    if (!step1 || !step1.zkp_x1 || step1.zkp_x1.id != this._their_signerid
        || !step1.zkp_x2 || step1.zkp_x2.id != this._their_signerid) {
      this._log.error("Invalid round 1 payload: " + JSON.stringify(step1));
      this.abort(JPAKE_ERROR_WRONGMESSAGE);
      return;
    }

    let A = {};
    let gvA = {};
    let rA = {};

    try {
      this._jpake.round2(this._their_signerid, this._secret,
                         step1.gx1, step1.zkp_x1.gr, step1.zkp_x1.b,
                         step1.gx2, step1.zkp_x2.gr, step1.zkp_x2.b,
                         A, gvA, rA);
    } catch (ex) {
      this._log.error("JPAKE round 2 threw: " + ex);
      this.abort(JPAKE_ERROR_INTERNAL);
      return;
    }
    let two = {A: A.value,
               zkp_A: {gr: gvA.value, b: rA.value, id: this._my_signerid}};
    this._outgoing = {type: this._my_signerid + "2", payload: two};
    this._log.trace("Generated message " + this._outgoing.type);
    callback();
  },

  _computeFinal: function _computeFinal(callback) {
    if (this._incoming.type != this._their_signerid + "2") {
      this._log.error("Invalid round 2 message: "
                      + JSON.stringify(this._incoming));
      this.abort(JPAKE_ERROR_WRONGMESSAGE);
      return;
    }

    let step2 = this._incoming.payload;
    if (!step2 || !step2.zkp_A || step2.zkp_A.id != this._their_signerid) {
      this._log.error("Invalid round 2 payload: " + JSON.stringify(step1));
      this.abort(JPAKE_ERROR_WRONGMESSAGE);
      return;
    }

    let aes256Key = {};
    let hmac256Key = {};

    try {
      this._jpake.final(step2.A, step2.zkp_A.gr, step2.zkp_A.b, HMAC_INPUT,
                        aes256Key, hmac256Key);
    } catch (ex) {
      this._log.error("JPAKE final round threw: " + ex);
      this.abort(JPAKE_ERROR_INTERNAL);
      return;
    }

    this._crypto_key = aes256Key.value;
    this._hmac_key = Utils.makeHMACKey(Utils.safeAtoB(hmac256Key.value));

    callback();
  },

  _computeKeyVerification: function _computeKeyVerification(callback) {
    this._log.trace("Encrypting key verification value.");
    let iv, ciphertext;
    try {
      iv = Svc.Crypto.generateRandomIV();
      ciphertext = Svc.Crypto.encrypt(JPAKE_VERIFY_VALUE,
                                      this._crypto_key, iv);
    } catch (ex) {
      this._log.error("Failed to encrypt key verification value.");
      this.abort(JPAKE_ERROR_INTERNAL);
      return;
    }
    this._outgoing = {type: this._my_signerid + "3",
                      payload: {ciphertext: ciphertext, IV: iv}};
    this._log.trace("Generated message " + this._outgoing.type);
    callback();
  },

  _encryptData: function _encryptData(callback) {
    this._log.trace("Verifying their key.");
    if (this._incoming.type != this._their_signerid + "3") {
      this._log.error("Invalid round 3 data: " +
                      JSON.stringify(this._incoming));
      this.abort(JPAKE_ERROR_WRONGMESSAGE);
      return;
    }
    let step3 = this._incoming.payload;
    try {
      ciphertext = Svc.Crypto.encrypt(JPAKE_VERIFY_VALUE,
                                      this._crypto_key, step3.IV);
      if (ciphertext != step3.ciphertext)
        throw "Key mismatch!";
    } catch (ex) {
      this._log.error("Keys don't match!");
      this.abort(JPAKE_ERROR_KEYMISMATCH);
      return;
    }

    this._log.trace("Encrypting data.");
    let iv, ciphertext, hmac;
    try {
      iv = Svc.Crypto.generateRandomIV();
      ciphertext = Svc.Crypto.encrypt(this._data, this._crypto_key, iv);
      hmac = Utils.sha256HMAC(ciphertext, this._hmac_key);
    } catch (ex) {
      this._log.error("Failed to encrypt data.");
      this.abort(JPAKE_ERROR_INTERNAL);
      return;
    }
    this._outgoing = {type: this._my_signerid + "3",
                      payload: {ciphertext: ciphertext, IV: iv, hmac: hmac}};
    this._log.trace("Generated message " + this._outgoing.type);
    callback();
  },

  _decryptData: function _decryptData(callback) {
    this._log.trace("Verifying their key.");
    if (this._incoming.type != this._their_signerid + "3") {
      this._log.error("Invalid round 3 data: "
                      + JSON.stringify(this._incoming));
      this.abort(JPAKE_ERROR_WRONGMESSAGE);
      return;
    }
    let step3 = this._incoming.payload;
    try {
      let hmac = Utils.sha256HMAC(step3.ciphertext, this._hmac_key);
      if (hmac != step3.hmac)
        throw "HMAC validation failed!";
    } catch (ex) {
      this._log.error("HMAC validation failed.");
      this.abort(JPAKE_ERROR_KEYMISMATCH);
      return;
    }

    this._log.trace("Decrypting data.");
    let cleartext;
    try {      
      cleartext = Svc.Crypto.decrypt(step3.ciphertext, this._crypto_key,
                                     step3.IV);
    } catch (ex) {
      this._log.error("Failed to decrypt data.");
      this.abort(JPAKE_ERROR_INTERNAL);
      return;
    }

    try {
      this._newData = JSON.parse(cleartext);
    } catch (ex) {
      this._log.error("Invalid data data: " + JSON.stringify(cleartext));
      this.abort(JPAKE_ERROR_INVALID);
      return;
    }

    this._log.trace("Decrypted data.");
    callback();
  },

  _complete: function _complete() {
    this._log.debug("Exchange completed.");
    this._finished = true;
    Utils.delay(function () { this.observer.onComplete(this._newData); },
                0, this, "_timer_onComplete");
  }

};
