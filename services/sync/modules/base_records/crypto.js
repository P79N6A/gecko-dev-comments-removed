



































const EXPORTED_SYMBOLS = ['CryptoWrapper', 'CryptoMeta', 'CryptoMetas'];

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
Cu.import("resource://weave/base_records/wbo.js");
Cu.import("resource://weave/base_records/keys.js");

Function.prototype.async = Async.sugar;

Utils.lazy(this, 'CryptoMetas', RecordManager);


let json = Cc["@mozilla.org/dom/json;1"].createInstance(Ci.nsIJSON);
let crypto = Cc["@labs.mozilla.com/Weave/Crypto;1"].createInstance(Ci.IWeaveCrypto);

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

  get encryption() this.payload.encryption,
  set encryption(value) {
    this.payload.encryption = value;
  },

  get ciphertext() this.payload.ciphertext,
  set ciphertext(value) {
    this.payload.ciphertext = value;
  },

  _encrypt: function CryptoWrap__encrypt(passphrase) {
    let self = yield;

    let pubkey = yield PubKeys.getDefaultKey(self.cb);
    let privkey = yield PrivKeys.get(self.cb, pubkey.privateKeyUri);

    let meta = yield CryptoMetas.get(self.cb, this.encryption);
    let symkey = yield meta.getKey(self.cb, privkey, passphrase);

    this.ciphertext = crypto.encrypt(json.encode([this.cleartext]), symkey, meta.bulkIV);
    this.cleartext = null;

    self.done();
  },
  encrypt: function CryptoWrap_encrypt(onComplete, passphrase) {
    this._encrypt.async(this, onComplete, passphrase);
  },

  _decrypt: function CryptoWrap__decrypt(passphrase) {
    let self = yield;

    let pubkey = yield PubKeys.getDefaultKey(self.cb);
    let privkey = yield PrivKeys.get(self.cb, pubkey.privateKeyUri);

    let meta = yield CryptoMetas.get(self.cb, this.encryption);
    let symkey = yield meta.getKey(self.cb, privkey, passphrase);

    
    this.cleartext = json.decode(crypto.decrypt(this.ciphertext, symkey, meta.bulkIV))[0];
    this.ciphertext = null;

    self.done(this.cleartext);
  },
  decrypt: function CryptoWrap_decrypt(onComplete, passphrase) {
    this._decrypt.async(this, onComplete, passphrase);
  }
};

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

function RecordManager() {
  this._init();
}
RecordManager.prototype = {
  _recordType: CryptoMeta,
  _logName: "RecordMgr",

  _init: function RegordMgr__init() {
    this._log = Log4Moz.repository.getLogger(this._logName);
    this._records = {};
    this._aliases = {};
  },

  _import: function RegordMgr__import(url) {
    let self = yield;
    let rec;

    this._log.trace("Importing record: " + (url.spec? url.spec : url));

    try {
      rec = new this._recordType(url);
      yield rec.get(self.cb);
      this.set(url, rec);
    } catch (e) {
      this._log.debug("Failed to import record: " + e);
      rec = null;
    }
    self.done(rec);
  },
  import: function RegordMgr_import(onComplete, url) {
    this._import.async(this, onComplete, url);
  },

  _get: function RegordMgr__get(url) {
    let self = yield;

    let rec = null;
    if (url in this._aliases)
      url = this._aliases[url];
    if (url in this._records)
      rec = this._records[url];

    if (!rec)
      rec = yield this.import(self.cb, url);

    self.done(rec);
  },
  get: function RegordMgr_get(onComplete, url) {
    this._get.async(this, onComplete, url);
  },

  set: function RegordMgr_set(url, record) {
    this._records[url] = record;
  },

  contains: function RegordMgr_contains(url) {
    let record = null;
    if (url in this._aliases)
      url = this._aliases[url];
    if (url in this._records)
      return true;
    return false;
  },

  del: function RegordMgr_del(url) {
    delete this._records[url];
  },
  getAlias: function RegordMgr_getAlias(alias) {
    return this._aliases[alias];
  },
  setAlias: function RegordMgr_setAlias(url, alias) {
    this._aliases[alias] = url;
  },
  delAlias: function RegordMgr_delAlias(alias) {
    delete this._aliases[alias];
  }
};
