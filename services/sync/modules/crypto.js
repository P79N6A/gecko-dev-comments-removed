



































const EXPORTED_SYMBOLS = ['WeaveCrypto'];

const Cc = Components.classes;
const Ci = Components.interfaces;
const Cr = Components.results;
const Cu = Components.utils;

Cu.import("resource://weave/log4moz.js");
Cu.import("resource://weave/constants.js");
Cu.import("resource://weave/util.js");

Cu.import("resource://gre/modules/XPCOMUtils.jsm");

function WeaveCrypto() {
  this._init();
}
WeaveCrypto.prototype = {
  _logName: "Crypto",

  __os: null,
  get _os() {
    if (!this.__os)
      this.__os = Cc["@mozilla.org/observer-service;1"]
        .getService(Ci.nsIObserverService);
    return this.__os;
  },

  __xxxtea: {},
  __xxxteaLoaded: false,
  get _xxxtea() {
    if (!this.__xxxteaLoaded) {
      let jsLoader = Cc["@mozilla.org/moz/jssubscript-loader;1"].
        getService(Ci.mozIJSSubScriptLoader);
      jsLoader.loadSubScript("chrome://weave/content/encrypt.js", this.__xxxtea);
      this.__xxxteaLoaded = true;
    }
    return this.__xxxtea;
  },

  get defaultAlgorithm() {
    let branch = Cc["@mozilla.org/preferences-service;1"]
      .getService(Ci.nsIPrefBranch);
    return branch.getCharPref("browser.places.sync.encryption");
  },
  set defaultAlgorithm(value) {
    let branch = Cc["@mozilla.org/preferences-service;1"]
      .getService(Ci.nsIPrefBranch);
    let cur = branch.getCharPref("browser.places.sync.encryption");
    if (value != cur)
      branch.setCharPref("browser.places.sync.encryption", value);
  },

  _init: function Crypto__init() {
    this._log = Log4Moz.Service.getLogger("Service." + this._logName);
    let branch = Cc["@mozilla.org/preferences-service;1"]
      .getService(Ci.nsIPrefBranch2);
    branch.addObserver("browser.places.sync.encryption", this, false);
  },

  QueryInterface: XPCOMUtils.generateQI([Ci.nsIObserver, Ci.nsISupports]),

  

  observe: function Sync_observe(subject, topic, data) {
    switch (topic) {
    case "browser.places.sync.encryption":
      let branch = Cc["@mozilla.org/preferences-service;1"]
        .getService(Ci.nsIPrefBranch);

      let cur = branch.getCharPref("browser.places.sync.encryption");
      if (cur == data)
	return;

      switch (data) {
      case "none":
        this._log.info("Encryption disabled");
        break;
      case "XXXTEA":
        this._log.info("Using encryption algorithm: " + data);
        break;
      default:
	this._log.warn("Unknown encryption algorithm, resetting");
	branch.setCharPref("browser.places.sync.encryption", "XXXTEA");
	return; 
      }
      
      this._os.notifyObservers(null, "weave:encryption:algorithm-changed", "");
      break;
    default:
      this._log.warn("Unknown encryption preference changed - ignoring");
    }
  },

  

  PBEencrypt: function Crypto_PBEencrypt(data, identity, algorithm) {
    let out;
    if (!algorithm)
      algorithm = this.defaultAlgorithm;
    switch (algorithm) {
    case "none":
      out = data;
      break;
    case "XXXTEA":
      try {
        this._log.debug("Encrypting data");
        out = this._xxxtea.encrypt(data, identity.password);
        this._log.debug("Done encrypting data");
      } catch (e) {
        this._log.error("Data encryption failed: " + e);
        throw 'encrypt failed';
      }
      break;
    default:
      this._log.error("Unknown encryption algorithm: " + algorithm);
      throw 'encrypt failed';
    }
    return out;
  },

  PBEdecrypt: function Crypto_PBEdecrypt(data, identity, algorithm) {
    let out;
    switch (algorithm) {
    case "none":
      out = eval(data);
      break;
    case "XXXTEA":
      try {
        this._log.debug("Decrypting data");
        out = eval(this._xxxtea.decrypt(data, identity.password));
        this._log.debug("Done decrypting data");
      } catch (e) {
        this._log.error("Data decryption failed: " + e);
        throw 'decrypt failed';
      }
      break;
    default:
      this._log.error("Unknown encryption algorithm: " + algorithm);
      throw 'decrypt failed';
    }
    return out;
  }
};
