



































const EXPORTED_SYMBOLS = ['PubKey', 'PrivKey',
                          'PubKeys', 'PrivKeys'];

const Cc = Components.classes;
const Ci = Components.interfaces;
const Cr = Components.results;
const Cu = Components.utils;

Cu.import("resource://services-sync/base_records/wbo.js");
Cu.import("resource://services-sync/constants.js");
Cu.import("resource://services-sync/log4moz.js");
Cu.import("resource://services-sync/resource.js");
Cu.import("resource://services-sync/util.js");

function PubKey(uri) {
  WBORecord.call(this, uri);
  this.type = "pubkey";
  this.keyData = null;
}
PubKey.prototype = {
  __proto__: WBORecord.prototype,
  _logName: "Record.PubKey",

  get privateKeyUri() {
    if (!this.data)
      return null;

    
    let key = this.payload.privateKeyUri;
    return Utils.makeURI(this.uri.resolve(key) || key);
  },
  set privateKeyUri(value) {
    this.payload.privateKeyUri = this.uri.getRelativeSpec(Utils.makeURI(value));
  },

  get publicKeyUri() {
    throw "attempted to get public key url from a public key!";
  }
};

Utils.deferGetSet(PubKey, "payload", ["keyData", "type"]);

function PrivKey(uri) {
  WBORecord.call(this, uri);
  this.type = "privkey";
  this.salt = null;
  this.iv = null;
  this.keyData = null;
}
PrivKey.prototype = {
  __proto__: WBORecord.prototype,
  _logName: "Record.PrivKey",

  get publicKeyUri() {
    if (!this.data)
      return null;

    
    let key = this.payload.publicKeyUri;
    return Utils.makeURI(this.uri.resolve(key) || key);
  },
  set publicKeyUri(value) {
    this.payload.publicKeyUri = this.uri.getRelativeSpec(Utils.makeURI(value));
  },

  get privateKeyUri() {
    throw "attempted to get private key url from a private key!";
  }
};

Utils.deferGetSet(PrivKey, "payload", ["salt", "iv", "keyData", "type"]);


function SymKey(keyData, wrapped) {
  this._data = keyData;
  this._wrapped = wrapped;
}
SymKey.prototype = {
  get wrapped() {
    return this._wrapped;
  },

  unwrap: function SymKey_unwrap(privkey, passphrase, meta_record) {
    this._data =
      Svc.Crypto.unwrapSymmetricKey(this._data, privkey.keyData, passphrase,
                                    privkey.salt, privkey.iv);
  }
};

Utils.lazy(this, 'PubKeys', PubKeyManager);

function PubKeyManager() {
  RecordManager.call(this);
}
PubKeyManager.prototype = {
  __proto__: RecordManager.prototype,
  _recordType: PubKey,
  _logName: "PubKeyManager",

  get defaultKeyUri() this._defaultKeyUri,
  set defaultKeyUri(value) { this._defaultKeyUri = value; },

  getDefaultKey: function PubKeyManager_getDefaultKey() {
    return this.get(this.defaultKeyUri);
  },

  createKeypair: function KeyMgr_createKeypair(passphrase, pubkeyUri, privkeyUri) {
    if (!pubkeyUri)
      throw "Missing or null parameter 'pubkeyUri'.";
    if (!privkeyUri)
      throw "Missing or null parameter 'privkeyUri'.";

    this._log.debug("Generating RSA keypair");
    let pubkey = new PubKey(pubkeyUri);
    let privkey = new PrivKey(privkeyUri);
    privkey.salt = Svc.Crypto.generateRandomBytes(16);
    privkey.iv = Svc.Crypto.generateRandomIV();

    let pub = {}, priv = {};
    Svc.Crypto.generateKeypair(passphrase.passwordUTF8, privkey.salt,
                               privkey.iv, pub, priv);
    [pubkey.keyData, privkey.keyData] = [pub.value, priv.value];

    pubkey.privateKeyUri = privkeyUri;
    privkey.publicKeyUri = pubkeyUri;

    this._log.debug("Generating RSA keypair... done");
    return {pubkey: pubkey, privkey: privkey};
  },

  uploadKeypair: function PubKeyManager_uploadKeypair(keys) {
    for each (let key in keys)
      new Resource(key.uri).put(key);
  }
};

Utils.lazy(this, 'PrivKeys', PrivKeyManager);

function PrivKeyManager() {
  PubKeyManager.call(this);
}
PrivKeyManager.prototype = {
  __proto__: PubKeyManager.prototype,
  _recordType: PrivKey,
  _logName: "PrivKeyManager"
};
