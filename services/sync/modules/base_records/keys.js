



































const EXPORTED_SYMBOLS = ['PubKey', 'PrivKey',
                          'PubKeys', 'PrivKeys'];

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

Function.prototype.async = Async.sugar;

Utils.lazy(this, 'PubKeys', PubKeyManager);
Utils.lazy(this, 'PrivKeys', PrivKeyManager);

function PubKey(uri, authenticator) {
  this._PubKey_init(uri, authenticator);
}
PubKey.prototype = {
  __proto__: WBORecord.prototype,

  _PubKey_init: function PubKey_init(uri, authenticator) {
    this._WBORec_init(uri, authenticator);
  },

  get keyData() {
    if (!this.data)
      return null;
    return this.data.payload.key_data;
  },

  get privateKeyUri() {
    if (!this.data)
      return null;
    
    let uri = this.uri.resolve(this.data.payload.private_key);
    if (uri)
      return Utils.makeURI(uri);
    
    return Utils.makeURI(this.data.payload.private_key);
  }
};

function PrivKey(uri, authenticator) {
  this._PrivKey_init(uri, authenticator);
}
PrivKey.prototype = {
  __proto__: WBORecord.prototype,

  _PrivKey_init: function PrivKey_init(uri, authenticator) {
    this._WBORec_init(uri, authenticator);
  },

  get keyData() {
    if (!this.data)
      return null;
    return this.data.payload.key_data;
  },

  get privateKeyUri() {
    throw "attempted to get private key url from a private key!";
  },

  get publicKeyUri() {
    if (!this.data)
      return null;
    
    let uri = this.uri.resolve(this.data.payload.public_key);
    if (uri)
      return Utils.makeURI(uri);
    
    return Utils.makeURI(this.data.payload.public_key);
  }
};

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

function PubKeyManager() {
  this._init();
}
PubKeyManager.prototype = {
  _keyType: PubKey,
  _logName: "PubKeyManager",

  get defaultKeyUri() this._defaultKeyUrl,
  set defaultKeyUri(value) { this._defaultKeyUri = value; },

  _getDefaultKey: function KeyMgr__getDefaultKey() {
    let self = yield;
    let ret = yield this.get(self.cb, this.defaultKeyUri);
    self.done(ret);
  },
  getDefaultKey: function KeyMgr_getDefaultKey(onComplete) {
    return this._getDefaultKey.async(this, onComplete);
  },

  _init: function KeyMgr__init() {
    this._log = Log4Moz.repository.getLogger(this._logName);
    this._keys = {};
    this._aliases = {};
  },

  _import: function KeyMgr__import(url) {
    let self = yield;
    let ret = null;

    this._log.trace("Importing key: " + (url.spec? url.spec : url));

    try {
      let key = new this._keyType(url);
      let foo = yield key.get(self.cb);
      this.set(url, key);
      ret = key;
    } catch (e) {
      this._log.debug("Failed to import key: " + e);
      
    }

    self.done(ret);
  },
  import: function KeyMgr_import(onComplete, url) {
    this._import.async(this, onComplete, url);
  },

  _get: function KeyMgr__get(url) {
    let self = yield;

    let key = null;
    if (url in this._aliases)
      url = this._aliases[url];
    if (url in this._keys)
      key = this._keys[url];

    if (!key)
      key = yield this.import(self.cb, url);

    self.done(key);
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

function PrivKeyManager() { this._init(); }
PrivKeyManager.prototype = {
  __proto__: PubKeyManager.prototype,
  _keyType: PrivKey,
  _logName: "PrivKeyManager"
};
