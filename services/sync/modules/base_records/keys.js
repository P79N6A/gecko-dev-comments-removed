



































const EXPORTED_SYMBOLS = ['PubKey', 'PrivKey',
                          'PubKeys', 'PrivKeys'];

const Cc = Components.classes;
const Ci = Components.interfaces;
const Cr = Components.results;
const Cu = Components.utils;

Cu.import("resource://weave/log4moz.js");
Cu.import("resource://weave/constants.js");
Cu.import("resource://weave/util.js");
Cu.import("resource://weave/async.js");
Cu.import("resource://weave/resource.js");
Cu.import("resource://weave/base_records/wbo.js");

Function.prototype.async = Async.sugar;

function PubKey(uri) {
  this._PubKey_init(uri);
}
PubKey.prototype = {
  __proto__: WBORecord.prototype,
  _logName: "Record.PubKey",

  _PubKey_init: function PubKey_init(uri) {
    this._WBORec_init(uri);
    this.payload = {
      type: "pubkey",
      keyData: null,
      privateKeyUri: null
    };
  },

  get privateKeyUri() {
    if (!this.data)
      return null;

    
    let key = this.payload.privateKeyUri;
    return Utils.makeURI(this.uri.resolve(key) || key);
  },

  get publicKeyUri() {
    throw "attempted to get public key url from a public key!";
  }
};

Utils.deferGetSet(PubKey, "payload", ["keyData", "privateKeyUri"]);

function PrivKey(uri) {
  this._PrivKey_init(uri);
}
PrivKey.prototype = {
  __proto__: WBORecord.prototype,
  _logName: "Record.PrivKey",

  _PrivKey_init: function PrivKey_init(uri) {
    this._WBORec_init(uri);
    this.payload = {
      type: "privkey",
      salt: null,
      iv: null,
      keyData: null,
      publicKeyUri: null
    };
  },

  get publicKeyUri() {
    if (!this.data)
      return null;

    
    let key = this.payload.publicKeyUri;
    return Utils.makeURI(this.uri.resolve(key) || key);
  },

  get privateKeyUri() {
    throw "attempted to get private key url from a private key!";
  }
};

Utils.deferGetSet(PrivKey, "payload", ["salt", "iv", "keyData", "publicKeyUri"]);


function SymKey(keyData, wrapped) {
  this._init(keyData, wrapped);
}
SymKey.prototype = {
  get wrapped() {
    return this._wrapped;
  },

  _init: function SymKey__init(keyData, wrapped) {
    this._data = keyData;
    this._wrapped = wrapped;
  },

  unwrap: function SymKey_unwrap(privkey, passphrase, meta_record) {
    let svc = Cc["@labs.mozilla.com/Weave/Crypto;1"].
      createInstance(Ci.IWeaveCrypto);
    this._data = svc.unwrapSymmetricKey(this._data,
                                        privkey.keyData,
                                        passphrase,
                                        identity.passphraseSalt,
                                        identity.privkeyWrapIV);
  }
};

Utils.lazy(this, 'PubKeys', PubKeyManager);

function PubKeyManager() { this._init(); }
PubKeyManager.prototype = {
  __proto__: RecordManager.prototype,
  _recordType: PubKey,
  _logName: "PubKeyManager",

  get defaultKeyUri() this._defaultKeyUri,
  set defaultKeyUri(value) { this._defaultKeyUri = value; },

  getDefaultKey: function KeyMgr_getDefaultKey(onComplete) {
    let fn = function KeyMgr__getDefaultKey() {
      let self = yield;
      let ret = yield this.get(self.cb, this.defaultKeyUri);
      self.done(ret);
    };
    fn.async(this, onComplete);
  },

  createKeypair: function KeyMgr_createKeypair(passphrase, pubkeyUri, privkeyUri) {
    this._log.debug("Generating RSA keypair");
    let pubkey = new PubKey();
    let privkey = new PrivKey();
    privkey.salt = Svc.Crypto.generateRandomBytes(16);
    privkey.iv = Svc.Crypto.generateRandomIV();

    let pub = {}, priv = {};
    Svc.Crypto.generateKeypair(passphrase, privkey.salt, privkey.iv, pub, priv);
    [pubkey.keyData, privkey.keyData] = [pub.value, priv.value];

    if (pubkeyUri) {
      pubkey.uri = pubkeyUri;
      privkey.publicKeyUri = pubkeyUri;
    }
    if (privkeyUri) {
      privkey.uri = privkeyUri;
      pubkey.privateKeyUri = privkeyUri;
    }

    this._log.debug("Generating RSA keypair... done");
    return {pubkey: pubkey, privkey: privkey};
  },

  uploadKeypair: function KeyMgr_uploadKeypair(onComplete, keys) {
    let fn = function KeyMgr__uploadKeypair(keys) {
      let self = yield;
      for each (let key in keys) {
	let res = new Resource(key.uri);
	yield res.put(self.cb, key.serialize());
      }
    };
    fn.async(this, onComplete, keys);
  }
};

Utils.lazy(this, 'PrivKeys', PrivKeyManager);

function PrivKeyManager() { this._init(); }
PrivKeyManager.prototype = {
  __proto__: PubKeyManager.prototype,
  _recordType: PrivKey,
  _logName: "PrivKeyManager"
};
