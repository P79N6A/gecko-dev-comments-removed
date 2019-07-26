



"use strict";

this.EXPORTED_SYMBOLS = [
  "BulkKeyBundle",
  "SyncKeyBundle"
];

const {classes: Cc, interfaces: Ci, utils: Cu, results: Cr} = Components;

Cu.import("resource://services-sync/constants.js");
Cu.import("resource://gre/modules/Log.jsm");
Cu.import("resource://services-sync/util.js");











function KeyBundle() {
  this._encrypt = null;
  this._encryptB64 = null;
  this._hmac = null;
  this._hmacB64 = null;
  this._hmacObj = null;
  this._sha256HMACHasher = null;
}
KeyBundle.prototype = {
  _encrypt: null,
  _encryptB64: null,
  _hmac: null,
  _hmacB64: null,
  _hmacObj: null,
  _sha256HMACHasher: null,

  equals: function equals(bundle) {
    return bundle &&
           (bundle.hmacKey == this.hmacKey) &&
           (bundle.encryptionKey == this.encryptionKey);
  },

  


  get encryptionKey() {
    return this._encrypt;
  },

  set encryptionKey(value) {
    if (!value || typeof value != "string") {
      throw new Error("Encryption key can only be set to string values.");
    }

    if (value.length < 16) {
      throw new Error("Encryption key must be at least 128 bits long.");
    }

    this._encrypt = value;
    this._encryptB64 = btoa(value);
  },

  get encryptionKeyB64() {
    return this._encryptB64;
  },

  get hmacKey() {
    return this._hmac;
  },

  set hmacKey(value) {
    if (!value || typeof value != "string") {
      throw new Error("HMAC key can only be set to string values.");
    }

    if (value.length < 16) {
      throw new Error("HMAC key must be at least 128 bits long.");
    }

    this._hmac = value;
    this._hmacB64 = btoa(value);
    this._hmacObj = value ? Utils.makeHMACKey(value) : null;
    this._sha256HMACHasher = value ? Utils.makeHMACHasher(
      Ci.nsICryptoHMAC.SHA256, this._hmacObj) : null;
  },

  get hmacKeyB64() {
    return this._hmacB64;
  },

  get hmacKeyObject() {
    return this._hmacObj;
  },

  get sha256HMACHasher() {
    return this._sha256HMACHasher;
  },

  


  generateRandom: function generateRandom() {
    let generatedHMAC = Svc.Crypto.generateRandomKey();
    let generatedEncr = Svc.Crypto.generateRandomKey();
    this.keyPairB64 = [generatedEncr, generatedHMAC];
  },

};






this.BulkKeyBundle = function BulkKeyBundle(collection) {
  let log = Log.repository.getLogger("Sync.BulkKeyBundle");
  log.info("BulkKeyBundle being created for " + collection);
  KeyBundle.call(this);

  this._collection = collection;
}

BulkKeyBundle.prototype = {
  __proto__: KeyBundle.prototype,

  get collection() {
    return this._collection;
  },

  




  get keyPair() {
    return [this.encryptionKey, this.hmacKey];
  },

  set keyPair(value) {
    if (!Array.isArray(value) || value.length != 2) {
      throw new Error("BulkKeyBundle.keyPair value must be array of 2 keys.");
    }

    this.encryptionKey = value[0];
    this.hmacKey       = value[1];
  },

  get keyPairB64() {
    return [this.encryptionKeyB64, this.hmacKeyB64];
  },

  set keyPairB64(value) {
    if (!Array.isArray(value) || value.length != 2) {
      throw new Error("BulkKeyBundle.keyPairB64 value must be an array of 2 " +
                      "keys.");
    }

    this.encryptionKey  = Utils.safeAtoB(value[0]);
    this.hmacKey        = Utils.safeAtoB(value[1]);
  },
};










this.SyncKeyBundle = function SyncKeyBundle(username, syncKey) {
  let log = Log.repository.getLogger("Sync.SyncKeyBundle");
  log.info("SyncKeyBundle being created.");
  KeyBundle.call(this);

  this.generateFromKey(username, syncKey);
}
SyncKeyBundle.prototype = {
  __proto__: KeyBundle.prototype,

  


  generateFromKey: function generateFromKey(username, syncKey) {
    if (!username || (typeof username != "string")) {
      throw new Error("Sync Key cannot be generated from non-string username.");
    }

    if (!syncKey || (typeof syncKey != "string")) {
      throw new Error("Sync Key cannot be generated from non-string key.");
    }

    if (!Utils.isPassphrase(syncKey)) {
      throw new Error("Provided key is not a passphrase, cannot derive Sync " +
                      "Key Bundle.");
    }

    
    let prk = Utils.decodeKeyBase32(syncKey);
    let info = HMAC_INPUT + username;
    let okm = Utils.hkdfExpand(prk, info, 32 * 2);
    this.encryptionKey = okm.slice(0, 32);
    this.hmacKey = okm.slice(32, 64);
  },
};

