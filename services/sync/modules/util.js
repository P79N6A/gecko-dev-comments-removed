



































const EXPORTED_SYMBOLS = ['Utils'];

const Cc = Components.classes;
const Ci = Components.interfaces;
const Cr = Components.results;
const Cu = Components.utils;

Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://weave/constants.js");
Cu.import("resource://weave/log4moz.js");





let Utils = {

  
  
  lazy: function Weave_lazy(dest, prop, ctr) {
    let getter = function() {
      delete dest[prop];
      dest[prop] = new ctr();
      return dest[prop];
    };
    dest.__defineGetter__(prop, getter);
  },

  deepEquals: function Weave_deepEquals(a, b) {
    if (!a && !b)
      return true;
    if (!a || !b)
      return false;

    
    if (typeof(a) != "object" && typeof(b) != "object")
      return a == b;

    
    if (typeof(a) != "object" || typeof(b) != "object")
      return false;

    if ("Array" == a.constructor.name && "Array" == b.constructor.name) {
      if (a.length != b.length)
        return false;

      for (let i = 0; i < a.length; i++) {
        if (!Utils.deepEquals(a[i], b[i]))
          return false;
      }

    } else {
      
      if ("Array" == a.constructor.name || "Array" == b.constructor.name)
        return false;

      for (let key in a) {
        if (!Utils.deepEquals(a[key], b[key]))
          return false;
      }
    }

    return true;
  },

  deepCopy: function Weave_deepCopy(thing) {
    if (typeof(thing) != "object" || thing == null)
      return thing;
    let ret;

    if ("Array" == thing.constructor.name) {
      ret = [];
      for (let i = 0; i < thing.length; i++)
        ret.push(Utils.deepCopy(thing[i]));

    } else {
      ret = {};
      for (let key in thing)
        ret[key] = Utils.deepCopy(thing[key]);
    }

    return ret;
  },

  exceptionStr: function Weave_exceptionStr(e) {
    let message = e.message? e.message : e;
    let location = e.location? " (" + e.location + ")" : "";
    return message + location;
  },

  stackTrace: function Weave_stackTrace(stackFrame, str) {
    if (stackFrame.caller)
      str = Utils.stackTrace(stackFrame.caller, str);

    if (!str)
      str = "";
    str = stackFrame + "\n" + str;

    return str;
  },

  checkStatus: function Weave_checkStatus(code, msg, ranges) {
    if (!ranges)
      ranges = [[200,300]];

    for (let i = 0; i < ranges.length; i++) {
      rng = ranges[i];
      if (typeof(rng) == "object" && code >= rng[0] && code < rng[1])
        return true;
      else if (typeof(rng) == "number" && code == rng) {
        return true;
      }
    }

    if (msg) {
      let log = Log4Moz.Service.getLogger("Service.Util");
      log.error(msg + " Error code: " + code);
    }

    return false;
  },

  ensureStatus: function Weave_ensureStatus(args) {
    if (!Utils.checkStatus.apply(Utils, arguments))
      throw 'checkStatus failed';
  },

  sha1: function Weave_sha1(string) {
    let converter = Cc["@mozilla.org/intl/scriptableunicodeconverter"].
      createInstance(Ci.nsIScriptableUnicodeConverter);
    converter.charset = "UTF-8";

    let hasher = Cc["@mozilla.org/security/hash;1"]
      .createInstance(Ci.nsICryptoHash);
    hasher.init(hasher.SHA1);

    let data = converter.convertToByteArray(string, {});
    hasher.update(data, data.length);
    let rawHash = hasher.finish(false);

    
    function toHexString(charCode) {
      return ("0" + charCode.toString(16)).slice(-2);
    }

    let hash = [toHexString(rawHash.charCodeAt(i)) for (i in rawHash)].join("");
    return hash;
  },

  makeURI: function Weave_makeURI(URIString) {
    if (URIString === null || URIString == "")
      return null;
    let ioservice = Cc["@mozilla.org/network/io-service;1"].
      getService(Ci.nsIIOService);
    return ioservice.newURI(URIString, null, null);
  },

  xpath: function Weave_xpath(xmlDoc, xpathString) {
    let root = xmlDoc.ownerDocument == null ?
      xmlDoc.documentElement : xmlDoc.ownerDocument.documentElement;
    let nsResolver = xmlDoc.createNSResolver(root);

    return xmlDoc.evaluate(xpathString, xmlDoc, nsResolver,
                           Ci.nsIDOMXPathResult.ANY_TYPE, null);
  },

  runCmd: function Weave_runCmd() {
    var binary;
    var args = [];

    for (let i = 0; i < arguments.length; ++i) {
      args.push(arguments[i]);
    }

    if (args[0] instanceof Ci.nsIFile) {
      binary = args.shift();
    } else {
      binary = Cc["@mozilla.org/file/local;1"].createInstance(Ci.nsILocalFile);
      binary.initWithPath(args.shift());
    }

    var p = Cc["@mozilla.org/process/util;1"].createInstance(Ci.nsIProcess);
    p.init(binary);

    let log = Log4Moz.Service.getLogger("Service.Util");
    log.debug("Running command: " + binary.path + " " + args.join(" "));

    p.run(true, args, args.length);
    return p.exitValue;
  },

  getTmp: function Weave_getTmp(name) {
    let ds = Cc["@mozilla.org/file/directory_service;1"].
      getService(Ci.nsIProperties);

    let tmp = ds.get("ProfD", Ci.nsIFile);
    tmp.QueryInterface(Ci.nsILocalFile);

    tmp.append("weave");
    tmp.append("tmp");
    if (!tmp.exists())
      tmp.create(tmp.DIRECTORY_TYPE, PERMS_DIRECTORY);

    if (name)
      tmp.append(name);

    return tmp;
  },

  open: function open(pathOrFile, mode, perms) {
    let stream, file;

    if (pathOrFile instanceof Ci.nsIFile) {
      file = pathOrFile;
    } else {
      file = Cc["@mozilla.org/file/local;1"].createInstance(Ci.nsILocalFile);
      dump("PATH IS" + pathOrFile + "\n");
      file.initWithPath(pathOrFile);
    }

    if (!perms)
      perms = PERMS_FILE;

    switch(mode) {
    case "<": {
      if (!file.exists())
        throw "Cannot open file for reading, file does not exist";
      let fis = Cc["@mozilla.org/network/file-input-stream;1"].
        createInstance(Ci.nsIFileInputStream);
      fis.init(file, MODE_RDONLY, perms, 0);
      stream = Cc["@mozilla.org/intl/converter-input-stream;1"].
        createInstance(Ci.nsIConverterInputStream);
      stream.init(fis, "UTF-8", 4096,
                  Ci.nsIConverterInputStream.DEFAULT_REPLACEMENT_CHARACTER);
    } break;

    case ">": {
      let fos = Cc["@mozilla.org/network/file-output-stream;1"].
        createInstance(Ci.nsIFileOutputStream);
      fos.init(file, MODE_WRONLY | MODE_CREATE | MODE_TRUNCATE, perms, 0);
      stream = Cc["@mozilla.org/intl/converter-output-stream;1"]
        .createInstance(Ci.nsIConverterOutputStream);
      stream.init(fos, "UTF-8", 4096, 0x0000);
    } break;

    case ">>": {
      let fos = Cc["@mozilla.org/network/file-output-stream;1"].
        createInstance(Ci.nsIFileOutputStream);
      fos.init(file, MODE_WRONLY | MODE_CREATE | MODE_APPEND, perms, 0);
      stream = Cc["@mozilla.org/intl/converter-output-stream;1"]
        .createInstance(Ci.nsIConverterOutputStream);
      stream.init(fos, "UTF-8", 4096, 0x0000);
    } break;

    default:
      throw "Illegal mode to open(): " + mode;
    }

    return [stream, file];
  },

  
  readStream: function Weave_readStream(is) {
    let ret = "", str = {};
    while (is.readString(4096, str) != 0) {
      ret += str.value;
    }
    return ret;
  },

  bind2: function Async_bind2(object, method) {
    return function innerBind() { return method.apply(object, arguments); };
  },

  _prefs: null,
  get prefs() {
    if (!this.__prefs) {
      this.__prefs = Cc["@mozilla.org/preferences-service;1"]
        .getService(Ci.nsIPrefService);
      this.__prefs = this.__prefs.getBranch(PREFS_BRANCH);
      this.__prefs.QueryInterface(Ci.nsIPrefBranch2);
    }
    return this.__prefs;
  },

  




  EventListener: function Weave_EventListener(handler, eventName) {
    this._handler = handler;
    this._eventName = eventName;
    this._log = Log4Moz.Service.getLogger("Async.EventHandler");
    this._log.level =
      Log4Moz.Level[Utils.prefs.getCharPref("log.logger.async")];
  }
};

Utils.EventListener.prototype = {
  QueryInterface: XPCOMUtils.generateQI([Ci.nsITimerCallback, Ci.nsISupports]),

  
  handleEvent: function EL_handleEvent(event) {
    this._log.trace("Handling event " + this._eventName);
    this._handler(event);
  },

  
  notify: function EL_notify(timer) {
    
    this._handler(timer);
  }
}
