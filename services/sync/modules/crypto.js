



































const EXPORTED_SYMBOLS = ['WeaveCrypto'];

const Cc = Components.classes;
const Ci = Components.interfaces;
const Cr = Components.results;
const Cu = Components.utils;

Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://weave/log4moz.js");
Cu.import("resource://weave/constants.js");
Cu.import("resource://weave/util.js");

Function.prototype.async = Utils.generatorAsync;

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

  _openssl: function Crypto__openssl(op, algorithm, input, password) {
    let extMgr = Components.classes["@mozilla.org/extensions/manager;1"]
      .getService(Components.interfaces.nsIExtensionManager);
    let loc = extMgr.getInstallLocation("{340c2bbc-ce74-4362-90b5-7c26312808ef}");

    let wrap = loc.getItemLocation("{340c2bbc-ce74-4362-90b5-7c26312808ef}");
    wrap.append("openssl");
    let bin;

    let os = Cc["@mozilla.org/xre/app-info;1"].getService(Ci.nsIXULRuntime).OS;
    switch(os) {
    case "WINNT":
      wrap.append("win32");
      wrap.append("exec.bat");
      bin = wrap.parent.path + "\\openssl.exe";
      break;
    case "Linux":
    case "Darwin":
      wrap.append("unix");
      wrap.append("exec.sh");
      bin = "openssl";
      break;
    default:
      throw "encryption not supported on this platform: " + os;
    }

    let inputFile = Utils.getTmp("input");
    let [inputFOS] = Utils.open(inputFile, ">");
    inputFOS.write(input, input.length);
    inputFOS.close();

    let outputFile = Utils.getTmp("output");
    if (outputFile.exists())
      outputFile.remove(false);

    
    let passFile = Utils.getTmp("pass");
    let [passFOS] = Utils.open(passFile, ">", PERMS_PASSFILE);
    passFOS.write(password, password.length);
    passFOS.close();

    try {
      this._log.debug("Running command: " + wrap.path + " " +
                      Utils.getTmp().path + " " + bin + " " + algorithm + " " +
                      op + "-a -salt -in input -out output -pass file:pass");
      Utils.runCmd(wrap, Utils.getTmp().path, bin, algorithm, op, "-a", "-salt",
                   "-in", "input", "-out", "output", "-pass", "file:pass");
    } catch (e) {
      throw e;
    } finally {
      
      
    }

    let [outputFIS] = Utils.open(outputFile, "<");
    let ret = Utils.readStream(outputFIS);
    outputFIS.close();
    

    return ret;
  },

  QueryInterface: XPCOMUtils.generateQI([Ci.nsIObserver, Ci.nsISupports]),

  

  observe: function Sync_observe(subject, topic, data) {
    switch (topic) {
    case "extensions.weave.encryption": {
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
    } break;
    default:
      this._log.warn("Unknown encryption preference changed - ignoring");
    }
  },

  

  PBEencrypt: function Crypto_PBEencrypt(onComplete, data, identity, algorithm) {
    let [self, cont] = yield;
    let listener = new Utils.EventListener(cont);
    let timer = Cc["@mozilla.org/timer;1"].createInstance(Ci.nsITimer);
    let ret;

    try {
      if (!algorithm)
        algorithm = this.defaultAlgorithm;

      if (algorithm != "none")
        this._log.debug("Encrypting data");

      switch (algorithm) {
      case "none":
        ret = data;
        break;

      case "XXXTEA": 
      case "XXTEA": {
        let gen = this._xxtea.encrypt(data, identity.password);
        ret = gen.next();
        while (typeof(ret) == "object") {
          timer.initWithCallback(listener, 0, timer.TYPE_ONE_SHOT);
          yield; 
          ret = gen.next();
        }
        gen.close();
      } break;

      case "aes-128-cbc":
      case "aes-192-cbc":
      case "aes-256-cbc":
      case "bf-cbc":
      case "des-ede3-cbc":
        ret = this._openssl("-e", algorithm, data, identity.password);
        break;

      default:
        throw "Unknown encryption algorithm: " + algorithm;
      }

      if (algorithm != "none")
        this._log.debug("Done encrypting data");

    } catch (e) {
      this._log.error("Exception caught: " + (e.message? e.message : e));

    } finally {
      timer = null;
      Utils.generatorDone(this, self, onComplete, ret);
      yield; 
    }
    this._log.warn("generator not properly closed");
  },

  PBEdecrypt: function Crypto_PBEdecrypt(onComplete, data, identity, algorithm) {
    let [self, cont] = yield;
    let listener = new Utils.EventListener(cont);
    let timer = Cc["@mozilla.org/timer;1"].createInstance(Ci.nsITimer);
    let ret;

    try {
      if (!algorithm)
        algorithm = this.defaultAlgorithm;

      if (algorithm != "none")
        this._log.debug("Decrypting data");

      switch (algorithm) {
      case "none":
        ret = data;
        break;

      case "XXXTEA": 
      case "XXTEA": {
        let gen = this._xxtea.decrypt(data, identity.password);
        ret = gen.next();
        while (typeof(ret) == "object") {
          timer.initWithCallback(listener, 0, timer.TYPE_ONE_SHOT);
          yield; 
          ret = gen.next();
        }
        gen.close();
      } break;

      case "aes-128-cbc":
      case "aes-192-cbc":
      case "aes-256-cbc":
      case "bf-cbc":
      case "des-ede3-cbc":
        ret = this._openssl("-d", algorithm, data, identity.password);
        break;

      default:
        throw "Unknown encryption algorithm: " + algorithm;
      }

      if (algorithm != "none")
        this._log.debug("Done decrypting data");

    } catch (e) {
      this._log.error("Exception caught: " + (e.message? e.message : e));

    } finally {
      timer = null;
      Utils.generatorDone(this, self, onComplete, ret);
      yield; 
    }
    this._log.warn("generator not properly closed");
  }
};
