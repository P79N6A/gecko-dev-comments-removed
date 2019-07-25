



































const EXPORTED_SYMBOLS = ['CryptoWrapper', 'CryptoMeta', 'CryptoMetas'];

const Cc = Components.classes;
const Ci = Components.interfaces;
const Cr = Components.results;
const Cu = Components.utils;

Cu.import("resource://weave/log4moz.js");
Cu.import("resource://weave/constants.js");
Cu.import("resource://weave/util.js");
Cu.import("resource://weave/async.js");
Cu.import("resource://weave/base_records/wbo.js");
Cu.import("resource://weave/base_records/keys.js");

Function.prototype.async = Async.sugar;

function CryptoWrapper(uri) {
  this._CryptoWrap_init(uri);
}
CryptoWrapper.prototype = {
  __proto__: WBORecord.prototype,
  _logName: "Record.CryptoWrapper",

  _CryptoWrap_init: function CryptoWrap_init(uri) {
    
    
    this._WBORec_init(uri);
    this.data.payload = {
      encryption: "",
      ciphertext: null
    };
  },

  
  
  cleartext: null,

  _encrypt: function CryptoWrap__encrypt(passphrase) {
    let self = yield;

    
    if (this.deleted) {
      self.done();
      return;
    }

    let pubkey = yield PubKeys.getDefaultKey(self.cb);
    let privkey = PrivKeys.get(pubkey.privateKeyUri);

    let meta = CryptoMetas.get(this.encryption);
    let symkey = yield meta.getKey(self.cb, privkey, passphrase);

    this.ciphertext = Svc.Crypto.encrypt(JSON.stringify([this.cleartext]),
					 symkey, meta.bulkIV);
    this.cleartext = null;

    self.done();
  },
  encrypt: function CryptoWrap_encrypt(onComplete, passphrase) {
    this._encrypt.async(this, onComplete, passphrase);
  },

  _decrypt: function CryptoWrap__decrypt(passphrase) {
    let self = yield;

    
    if (this.deleted) {
      self.done();
      return;
    }

    let pubkey = yield PubKeys.getDefaultKey(self.cb);
    let privkey = PrivKeys.get(pubkey.privateKeyUri);

    let meta = CryptoMetas.get(this.encryption);
    let symkey = yield meta.getKey(self.cb, privkey, passphrase);

    
    this.cleartext = JSON.parse(Svc.Crypto.decrypt(this.ciphertext,
							symkey, meta.bulkIV))[0];
    this.ciphertext = null;

    self.done(this.cleartext);
  },
  decrypt: function CryptoWrap_decrypt(onComplete, passphrase) {
    this._decrypt.async(this, onComplete, passphrase);
  },

  toString: function CryptoWrap_toString() "{ " + [
      "id: " + this.id,
      "parent: " + this.parentid,
      "depth: " + this.depth + ", index: " + this.sortindex,
      "modified: " + this.modified,
      "payload: " + (this.deleted ? "DELETED" : JSON.stringify(this.cleartext))
    ].join("\n  ") + " }",
};

Utils.deferGetSet(CryptoWrapper, "payload", ["encryption", "ciphertext"]);

function CryptoMeta(uri) {
  this._CryptoMeta_init(uri);
}
CryptoMeta.prototype = {
  __proto__: WBORecord.prototype,
  _logName: "Record.CryptoMeta",

  _CryptoMeta_init: function CryptoMeta_init(uri) {
    this._WBORec_init(uri);
    this.data.payload = {
      bulkIV: null,
      keyring: {}
    };
  },

  generateIV: function CryptoMeta_generateIV() {
    this.bulkIV = Svc.Crypto.generateRandomIV();
  },

  _getKey: function CryptoMeta__getKey(privkey, passphrase) {
    let self = yield;
    let wrapped_key;

    
    if (this._unwrappedKey) {
      self.done(this._unwrappedKey);
      return;
    }

    
    let pubkeyUri = privkey.publicKeyUri.spec;

    
    for (let relUri in this.payload.keyring) {
      if (pubkeyUri == this.baseUri.resolve(relUri))
        wrapped_key = this.payload.keyring[relUri];
    }
    if (!wrapped_key)
      throw "keyring doesn't contain a key for " + pubkeyUri;

    this._unwrappedKey =
      Svc.Crypto.unwrapSymmetricKey(wrapped_key, privkey.keyData, passphrase,
                                    privkey.salt, privkey.iv);
    self.done(this._unwrappedKey);
  },
  getKey: function CryptoMeta_getKey(onComplete, privkey, passphrase) {
    this._getKey.async(this, onComplete, privkey, passphrase);
  },

  _addKey: function CryptoMeta__addKey(new_pubkey, privkey, passphrase) {
    let self = yield;
    let symkey = yield this.getKey(self.cb, privkey, passphrase);
    yield this.addUnwrappedKey(self.cb, new_pubkey, symkey);
  },
  addKey: function CryptoMeta_addKey(onComplete, new_pubkey, privkey, passphrase) {
    this._addKey.async(this, onComplete, new_pubkey, privkey, passphrase);
  },

  _addUnwrappedKey: function CryptoMeta__addUnwrappedKey(new_pubkey, symkey) {
    let self = yield;

    
    if (typeof new_pubkey == 'string')
      new_pubkey = PubKeys.get(self.cb, new_pubkey);

    
    
    for (let relUri in this.payload.keyring) {
      if (pubkeyUri == this.uri.resolve(relUri))
        delete this.payload.keyring[relUri];
    }

    this.payload.keyring[new_pubkey.uri.spec] =
      Svc.Crypto.wrapSymmetricKey(symkey, new_pubkey.keyData);
  },
  addUnwrappedKey: function CryptoMeta_addUnwrappedKey(onComplete, new_pubkey, symkey) {
    this._addUnwrappedKey.async(this, onComplete, new_pubkey, symkey);
  }
};

Utils.deferGetSet(CryptoMeta, "data.payload", "bulkIV");

Utils.lazy(this, 'CryptoMetas', CryptoRecordManager);

function CryptoRecordManager() { this._init(); }
CryptoRecordManager.prototype = {
  __proto__: RecordManager.prototype,
  _recordType: CryptoMeta
};
