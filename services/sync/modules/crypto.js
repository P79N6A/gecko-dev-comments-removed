



































const EXPORTED_SYMBOLS = ['WeaveCrypto'];

const Cc = Components.classes;
const Ci = Components.interfaces;
const Cr = Components.results;
const Cu = Components.utils;

Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://weave/log4moz.js");
Cu.import("resource://weave/constants.js");
Cu.import("resource://weave/util.js");

Function.prototype.async = generatorAsync;

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

  __xxtea: {},
  __xxteaLoaded: false,
  get _xxtea() {
    if (!this.__xxteaLoaded) {
      Cu.import("resource://weave/xxtea.js", this.__xxtea);
      this.__xxteaLoaded = true;
    }
    return this.__xxtea;
  },

  get defaultAlgorithm() {
    let branch = Cc["@mozilla.org/preferences-service;1"]
      .getService(Ci.nsIPrefBranch);
    return branch.getCharPref("extensions.weave.encryption");
  },
  set defaultAlgorithm(value) {
    let branch = Cc["@mozilla.org/preferences-service;1"]
      .getService(Ci.nsIPrefBranch);
    let cur = branch.getCharPref("extensions.weave.encryption");
    if (value != cur)
      branch.setCharPref("extensions.weave.encryption", value);
  },

  _init: function Crypto__init() {
    this._log = Log4Moz.Service.getLogger("Service." + this._logName);
    let branch = Cc["@mozilla.org/preferences-service;1"]
      .getService(Ci.nsIPrefBranch2);
    branch.addObserver("extensions.weave.encryption", this, false);
  },

  QueryInterface: XPCOMUtils.generateQI([Ci.nsIObserver, Ci.nsISupports]),

  

  observe: function Sync_observe(subject, topic, data) {
    switch (topic) {
    case "extensions.weave.encryption":
      let branch = Cc["@mozilla.org/preferences-service;1"]
        .getService(Ci.nsIPrefBranch);

      let cur = branch.getCharPref("extensions.weave.encryption");
      if (cur == data)
	return;

      switch (data) {
      case "none":
        this._log.info("Encryption disabled");
        break;
      case "XXTEA":
      case "XXXTEA": 
        this._log.info("Using encryption algorithm: " + data);
        break;
      default:
	this._log.warn("Unknown encryption algorithm, resetting");
	branch.setCharPref("extensions.weave.encryption", "XXTEA");
	return; 
      }
      
      this._os.notifyObservers(null, "weave:encryption:algorithm-changed", "");
      break;
    default:
      this._log.warn("Unknown encryption preference changed - ignoring");
    }
  },

  

  PBEencrypt: function Crypto_PBEencrypt(onComplete, data, identity, algorithm) {
    let [self, cont] = yield;
    let listener = new EventListener(cont);
    let timer = Cc["@mozilla.org/timer;1"].createInstance(Ci.nsITimer);
    let ret;

    try {
      if (!algorithm)
        algorithm = this.defaultAlgorithm;

      switch (algorithm) {
      case "none":
        ret = data;
      case "XXTEA":
      case "XXXTEA": 
        this._log.debug("Encrypting data");
        let gen = this._xxtea.encrypt(data, identity.password);
        ret = gen.next();
        while (typeof(ret) == "object") {
          timer.initWithCallback(listener, 0, timer.TYPE_ONE_SHOT);
          yield; 
          ret = gen.next();
        }
        gen.close();
        this._log.debug("Done encrypting data");
        break;
      default:
        throw "Unknown encryption algorithm: " + algorithm;
      }

    } catch (e) {
      this._log.error("Exception caught: " + (e.message? e.message : e));

    } finally {
      timer = null;
      generatorDone(this, self, onComplete, ret);
      yield; 
    }
    this._log.warn("generator not properly closed");
  },

  PBEdecrypt: function Crypto_PBEdecrypt(onComplete, data, identity, algorithm) {
    let [self, cont] = yield;
    let listener = new EventListener(cont);
    let timer = Cc["@mozilla.org/timer;1"].createInstance(Ci.nsITimer);
    let ret;

    try {
      if (!algorithm)
        algorithm = this.defaultAlgorithm;

      switch (algorithm) {
      case "none":
        ret = data;
        break;
      case "XXTEA":
      case "XXXTEA": 
        this._log.debug("Decrypting data");
        let gen = this._xxtea.decrypt(data, identity.password);
        ret = gen.next();
        while (typeof(ret) == "object") {
          timer.initWithCallback(listener, 0, timer.TYPE_ONE_SHOT);
          yield; 
          ret = gen.next();
        }
        gen.close();
        this._log.debug("Done decrypting data");
        break;
      default:
        throw "Unknown encryption algorithm: " + algorithm;
      }

    } catch (e) {
      this._log.error("Exception caught: " + (e.message? e.message : e));

    } finally {
      timer = null;
      generatorDone(this, self, onComplete, ret);
      yield; 
    }
    this._log.warn("generator not properly closed");
  }
};
