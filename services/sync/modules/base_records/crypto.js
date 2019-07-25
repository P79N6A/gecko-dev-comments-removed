



































const EXPORTED_SYMBOLS = ['CryptoWrapper'];

const Cc = Components.classes;
const Ci = Components.interfaces;
const Cr = Components.results;
const Cu = Components.utils;

Cu.import("resource://weave/Observers.js");
Cu.import("resource://weave/Preferences.js");
Cu.import("resource://weave/log4moz.js");
Cu.import("resource://weave/constants.js");
Cu.import("resource://weave/util.js");
Cu.import("resource://weave/async.js");
Cu.import("resource://weave/crypto.js");
Cu.import("resource://weave/base_records/wbo.js");
Cu.import("resource://weave/base_records/keys.js");

Function.prototype.async = Async.sugar;

function CryptoWrapper(uri, authenticator) {
  this._CryptoWrap_init(uri, authenticator);
}
CryptoWrapper.prototype = {
  __proto__: WBORecord.prototype,
  _logName: "Record.CryptoWrapper",

  _CryptoWrap_init: function CryptoWrap_init(uri, authenticator) {
    this._WBORec_init(uri, authenticator);
    this.data.payload = "";
  },

  _encrypt: function CryptoWrap__encrypt(passphrase) {
    let self = yield;

    let pubkey = yield PubKeys.getDefaultKey(self.cb);
    let privkey = yield PrivKeys.get(self.cb, pubkey.privateKeyUri);

    let meta = new CryptoMeta(this.data.encryption); 
    yield meta.get(self.cb);
    let symkey = meta.getKey(pubkey, privkey, passphrase);

    let crypto = Cc["@labs.mozilla.com/Weave/Crypto;1"].
      createInstance(Ci.IWeaveCrypto);
    this.data.payload = crypto.encrypt(this.cleartext, symkey, meta.iv);

    self.done();
  },
  encrypt: function CryptoWrap_encrypt(onComplete, passphrase) {
    this._encrypt.async(this, onComplete, passphrase);
  },

  _decrypt: function CryptoWrap__decrypt(passphrase) {
    let self = yield;

    let pubkey = yield PubKeys.getDefaultKey(self.cb);
    let privkey = yield PrivKeys.get(self.cb, pubkey.privateKeyUri);

    let meta = new CryptoMeta(this.data.encryption); 
    yield meta.get(self.cb);
    let symkey = meta.getKey(pubkey, privkey, passphrase);

    let crypto = Cc["@labs.mozilla.com/Weave/Crypto;1"].
      createInstance(Ci.IWeaveCrypto);
    this.cleartext = crypto.decrypt(this.data.payload, symkey, meta.iv);

    self.done(this.cleartext);
  },
  decrypt: function CryptoWrap_decrypt(onComplete, passphrase) {
    this._decrypt.async(this, onComplete, passphrase);
  }
};

function CryptoMeta(uri, authenticator) {
  this._CryptoMeta_init(uri, authenticator);
}
CryptoMeta.prototype = {
  __proto__: WBORecord.prototype,
  _logName: "Record.CryptoMeta",

  _CryptoMeta_init: function CryptoMeta_init(uri, authenticator) {
    this._WBORec_init(uri, authenticator);
    this.data.payload = {
      salt: null,
      iv: null,
      keyring: {}
    };
  },

  get salt() this.data.payload.salt,
  set salt(value) { this.data.payload.salt = value; },

  get iv() this.data.payload.iv,
  set iv(value) { this.data.payload.iv = value; },

  getKey: function getKey(pubKey, privKey, passphrase) {
    let wrappedKey;

    
    let pubKeyUri;
    if (typeof pubKey == 'string')
      pubKeyUri = this.uri.resolve(pubKey);
    else
      pubKeyUri = pubKey.spec;

    
    for (let relUri in this.data.payload.keyring) {
      if (pubKeyUri == this.uri.resolve(relUri))
        wrappedKey = this.data.payload.keyring[relUri];
    }
    if (!wrappedKey)
      throw "keyring doesn't contain a key for " + pubKeyUri;

    let crypto = Cc["@labs.mozilla.com/Weave/Crypto;1"].
      createInstance(Ci.IWeaveCrypto);
    return crypto.unwrapSymmetricKey(wrappedKey, privKey.keyData,
                                     passphrase, this.salt, this.iv);
  }
};