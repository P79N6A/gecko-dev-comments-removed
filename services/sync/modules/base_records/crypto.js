



































const EXPORTED_SYMBOLS = ['CryptoWrapper', 'CryptoMeta'];

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


let json = Cc["@mozilla.org/dom/json;1"].createInstance(Ci.nsIJSON);
let crypto = Cc["@labs.mozilla.com/Weave/Crypto;1"].createInstance(Ci.IWeaveCrypto);

function CryptoWrapper(uri, authenticator) {
  this._CryptoWrap_init(uri, authenticator);
}
CryptoWrapper.prototype = {
  __proto__: WBORecord.prototype,
  _logName: "Record.CryptoWrapper",

  _CryptoWrap_init: function CryptoWrap_init(uri, authenticator) {
    
    
    this._WBORec_init(uri, authenticator);
    this.data.encryption = "";
    this.data.payload = "";
  },

  
  
  cleartext: null,

  get encryption() this.data.encryption,
  set encryption(value) {
    this.data.encryption = value;
  },

  _encrypt: function CryptoWrap__encrypt(passphrase) {
    let self = yield;

    let pubkey = yield PubKeys.getDefaultKey(self.cb);
    let privkey = yield PrivKeys.get(self.cb, pubkey.privateKeyUri);

    let meta = new CryptoMeta(this.encryption); 
    yield meta.get(self.cb);
    let symkey = yield meta.getKey(self.cb, privkey, passphrase);

    this.payload = crypto.encrypt(json.encode([this.cleartext]), symkey, meta.bulkIV);

    self.done();
  },
  encrypt: function CryptoWrap_encrypt(onComplete, passphrase) {
    this._encrypt.async(this, onComplete, passphrase);
  },

  _decrypt: function CryptoWrap__decrypt(passphrase) {
    let self = yield;

    let pubkey = yield PubKeys.getDefaultKey(self.cb);
    let privkey = yield PrivKeys.get(self.cb, pubkey.privateKeyUri);

    let meta = new CryptoMeta(this.encryption); 
    yield meta.get(self.cb);
    let symkey = yield meta.getKey(self.cb, privkey, passphrase);

    this.cleartext = json.decode(crypto.decrypt(this.payload, symkey, meta.bulkIV))[0];

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
      bulkIV: null,
      keyring: {}
    };
  },

  generateIV: function CryptoMeta_generateIV() {
    this.bulkIV = crypto.generateRandomIV();
  },

  get bulkIV() this.data.payload.bulkIV,
  set bulkIV(value) { this.data.payload.bulkIV = value; },

  _getKey: function CryptoMeta__getKey(privkey, passphrase) {
    let self = yield;
    let wrapped_key;

    
    let pubkeyUri = privkey.publicKeyUri.spec;

    
    for (let relUri in this.payload.keyring) {
      if (pubkeyUri == this.uri.resolve(relUri))
        wrapped_key = this.payload.keyring[relUri];
    }
    if (!wrapped_key)
      throw "keyring doesn't contain a key for " + pubkeyUri;

    let ret = crypto.unwrapSymmetricKey(wrapped_key, privkey.keyData,
                                        passphrase, privkey.salt, privkey.iv);
    self.done(ret);
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
      crypto.wrapSymmetricKey(symkey, new_pubkey.keyData);
  },
  addUnwrappedKey: function CryptoMeta_addUnwrappedKey(onComplete, new_pubkey, symkey) {
    this._addUnwrappedKey.async(this, onComplete, new_pubkey, symkey);
  }
};

function CryptoMetaManager() {
  this._init();
}
CryptoMetaManager.prototype = {
  _init: function CryptoMetaMgr__init() {
    this._log = Log4Moz.repository.getLogger("CryptoMetaMgr");
    this._records = {};
    this._aliases = {};
  },

  _import: function CryptoMetaMgr__import(url) {
    let self = yield;

    this._log.trace("Importing record: " + (url.spec? url.spec : url));

    try {
      let rec = new CryptoMeta(url);
      yield rec.get(self.cb);
      this.set(url, rec);
      self.done(rec);
    } catch (e) {
      this._log.debug("Failed to import record: " + e);
      self.done(null);
    }
  },
  import: function CryptoMetaMgr_import(onComplete, url) {
    this._import.async(this, onComplete, url);
  },

  _get: function CryptoMetaMgr__get(url) {
    let self = yield;

    let rec = null;
    if (url in this._aliases)
      url = this._aliases[url];
    if (url in this._records)
      rec = this._keys[url];

    if (!key)
      rec = yield this.import(self.cb, url);

    self.done(rec);
  },
  get: function KeyMgr_get(onComplete, url) {
    this._get.async(this, onComplete, url);
  },

  set: function KeyMgr_set(url, key) {
    this._keys[url] = key;
    return key;
  },

  contains: function KeyMgr_contains(url) {
    let key = null;
    if (url in this._aliases)
      url = this._aliases[url];
    if (url in this._keys)
      return true;
    return false;
  },

  del: function KeyMgr_del(url) {
    delete this._keys[url];
  },
  getAlias: function KeyMgr_getAlias(alias) {
    return this._aliases[alias];
  },
  setAlias: function KeyMgr_setAlias(url, alias) {
    this._aliases[alias] = url;
  },
  delAlias: function KeyMgr_delAlias(alias) {
    delete this._aliases[alias];
  }
};
