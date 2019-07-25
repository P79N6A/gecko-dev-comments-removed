




































const EXPORTED_SYMBOLS = ['Crypto'];

const Cc = Components.classes;
const Ci = Components.interfaces;
const Cr = Components.results;
const Cu = Components.utils;

Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://weave/log4moz.js");
Cu.import("resource://weave/constants.js");
Cu.import("resource://weave/util.js");
Cu.import("resource://weave/async.js");

Function.prototype.async = Async.sugar;

Utils.lazy(this, 'Crypto', CryptoSvc);

function CryptoSvc() {
  this._init();
}
CryptoSvc.prototype = {
  _logName: "Crypto",

  __os: null,
  get _os() {
    if (!this.__os)
      this.__os = Cc["@mozilla.org/observer-service;1"]
        .getService(Ci.nsIObserverService);
    return this.__os;
  },

  __crypto: null,
  get _crypto() {
    if (!this.__crypto)
         this.__crypto = Cc["@labs.mozilla.com/Weave/Crypto;1"].
                         createInstance(Ci.IWeaveCrypto);
    return this.__crypto;
  },


  get defaultAlgorithm() {
    return Utils.prefs.getCharPref("encryption");
  },
  set defaultAlgorithm(value) {
    if (value != Utils.prefs.getCharPref("encryption"))
      Utils.prefs.setCharPref("encryption", value);
  },

  _init: function Crypto__init() {
    this._log = Log4Moz.repository.getLogger("Service." + this._logName);
    this._log.level =
      Log4Moz.Level[Utils.prefs.getCharPref("log.logger.service.crypto")];
    let branch = Cc["@mozilla.org/preferences-service;1"]
      .getService(Ci.nsIPrefBranch2);
    branch.addObserver("extensions.weave.encryption", this, false);
  },

  QueryInterface: XPCOMUtils.generateQI([Ci.nsIObserver, Ci.nsISupports]),

  

  observe: function Sync_observe(subject, topic, data) {
    switch (topic) {
    case "extensions.weave.encryption": {
      if (Utils.pref.getCharPref("encryption") == data)
        return;

      switch (data) {
        case "none":
          this._log.info("Encryption disabled");
          break;

        default:
          this._log.warn("Unknown encryption algorithm, resetting");
          branch.clearUserPref("extensions.weave.encryption");
          return; 
      }
      
      this._os.notifyObservers(null, "weave:encryption:algorithm-changed", "");
    } break;
    default:
      this._log.warn("Unknown encryption preference changed - ignoring");
    }
  },

  checkModule: function Crypto_checkModule() {
    let ok = false;

    try {
      let svc = Cc["@labs.mozilla.com/Weave/Crypto;1"].
        createInstance(Ci.IWeaveCrypto);
      let iv = svc.generateRandomIV();
      if (iv.length == 24)
        ok = true;

    } catch (e) {}

    return ok;
  },

  

  encryptData: function Crypto_encryptData(data, identity) {
    let self = yield;
    let ret;

    this._log.trace("encrypt called. [id=" + identity.realm + "]");

    if ("none" == this.defaultAlgorithm) {
      this._log.debug("NOT encrypting data");
      ret = data;
    } else {
      let symkey = identity.bulkKey;
      let iv     = identity.bulkIV;
      ret = this._crypto.encrypt(data, symkey, iv);
    }

    self.done(ret);
  },

  decryptData: function Crypto_decryptData(data, identity) {
    let self = yield;
    let ret;

    this._log.trace("decrypt called. [id=" + identity.realm + "]");

    if ("none" == this.defaultAlgorithm) {
      this._log.debug("NOT decrypting data");
      ret = data;
    } else {
      let symkey = identity.bulkKey;
      let iv     = identity.bulkIV;
      ret = this._crypto.decrypt(data, symkey, iv);
    }

    self.done(ret);
  },

  





  randomKeyGen: function Crypto_randomKeyGen(identity) {
    let self = yield;

    this._log.trace("randomKeyGen called. [id=" + identity.realm + "]");

    let symkey = this._crypto.generateRandomKey();
    let iv     = this._crypto.generateRandomIV();

    identity.bulkKey = symkey;
    identity.bulkIV  = iv;
  },

  





  RSAkeygen: function Crypto_RSAkeygen(identity) {
    let self = yield;

    this._log.trace("RSAkeygen called. [id=" + identity.realm + "]");
    let privOut = {};
    let pubOut  = {};

    
    
    let salt = this._crypto.generateRandomBytes(32);
    let iv   = this._crypto.generateRandomIV();

    this._crypto.generateKeypair(identity.password,
                                 salt, iv,
                                 pubOut, privOut);

    identity.keypairAlg     = "RSA";
    identity.pubkey         = pubOut.value;
    identity.privkey        = privOut.value;
    identity.passphraseSalt = salt;
    identity.privkeyWrapIV  = iv;
  },

  wrapKey : function Crypto_wrapKey(data, identity) {
    let self = yield;

    this._log.trace("wrapKey called. [id=" + identity.realm + "]");
    let ret = this._crypto.wrapSymmetricKey(data, identity.pubkey);

    self.done(ret);
  },

  unwrapKey: function Crypto_unwrapKey(data, identity) {
    let self = yield;

    this._log.trace("upwrapKey called. [id=" + identity.realm + "]");
    let ret = this._crypto.unwrapSymmetricKey(data,
                                              identity.privkey,
                                              identity.password,
                                              identity.passphraseSalt,
                                              identity.privkeyWrapIV);
    self.done(ret);
  },

  
  
  isPassphraseValid: function Crypto_isPassphraseValid(identity) {
    var self = yield;

    
    
    
    
    

    
    var idTemp = {realm: "Temporary passphrase validation"};

    
    this.randomKeyGen.async(Crypto, self.cb, idTemp);
    yield;

    
    this.wrapKey.async(Crypto, self.cb, idTemp.bulkKey, identity);
    let wrappedKey = yield;
    let unwrappedKey;

    
    try {
      this.unwrapKey.async(Crypto, self.cb, wrappedKey, identity);
      unwrappedKey = yield;
    } catch (e) {
      self.done(false);
      return;
    }

    
    
    if (unwrappedKey != idTemp.bulkKey)
      throw new Error("Unwrapped key is not identical to original key.");

    self.done(true);
  }
};
