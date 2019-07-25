



































const EXPORTED_SYMBOLS = ['Utils'];

const Cc = Components.classes;
const Ci = Components.interfaces;
const Cr = Components.results;
const Cu = Components.utils;

Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://weave/constants.js");
Cu.import("resource://weave/log4moz.js");





let Utils = {

  deepEquals: function Weave_deepEquals(a, b) {
    if (!a && !b)
      return true;
    if (!a || !b)
      return false;

    if (typeof(a) != "object" && typeof(b) != "object")
      return a == b;
    if (typeof(a) != "object" || typeof(b) != "object")
      return false;

    for (let key in a) {
      if (typeof(a[key]) == "object") {
        if (!typeof(b[key]) == "object")
          return false;
        if (!Utils.deepEquals(a[key], b[key]))
          return false;
      } else {
        if (a[key] != b[key])
          return false;
      }
    }
    return true;
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
      xmlDoc.documentElement : xmlDoc.ownerDocument.documentElement
    let nsResolver = xmlDoc.createNSResolver(root);

    return xmlDoc.evaluate(xpathString, xmlDoc, nsResolver,
                           Ci.nsIDOMXPathResult.ANY_TYPE, null);
  },

  bind2: function Weave_bind2(object, method) {
    return function innerBind() { return method.apply(object, arguments); }
  },

  
  
  
  
  
  
  
  
  
  
  

  generatorAsync: function Weave_generatorAsync(self, extra_args) {
    try {
      let args = Array.prototype.slice.call(arguments, 1);
      let gen = this.apply(self, args);
      gen.next(); 
      gen.send([gen, function(data) {Utils.continueGenerator(gen, data);}]);
      return gen;
    } catch (e) {
      if (e instanceof StopIteration) {
        dump("async warning: generator stopped unexpectedly");
        return null;
      } else {
        dump("Exception caught: " + e.message);
      }
    }
  },

  continueGenerator: function Weave_continueGenerator(generator, data) {
    try { generator.send(data); }
    catch (e) {
      if (e instanceof StopIteration)
        dump("continueGenerator warning: generator stopped unexpectedly");
      else
        dump("Exception caught: " + e.message);
    }
  },

  
  
  
  
  
  
  
  generatorDone: function Weave_generatorDone(object, generator, callback, retval) {
    if (object._timer)
      throw "Called generatorDone when there is a timer already set."

    let cb = Utils.bind2(object, function(event) {
      generator.close();
      generator = null;
      object._timer = null;
      if (callback)
        callback(retval);
    });

    object._timer = Cc["@mozilla.org/timer;1"].createInstance(Ci.nsITimer);
    object._timer.initWithCallback(new Utils.EventListener(cb),
                                   0, object._timer.TYPE_ONE_SHOT);
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
      stream = Cc["@mozilla.org/network/file-input-stream;1"].
        createInstance(Ci.nsIFileInputStream);
      stream.init(file, MODE_RDONLY, perms, 0);
      stream.QueryInterface(Ci.nsILineInputStream);
    } break;

    case ">": {
      stream = Cc["@mozilla.org/network/file-output-stream;1"].
        createInstance(Ci.nsIFileOutputStream);
      stream.init(file, MODE_WRONLY | MODE_CREATE | MODE_TRUNCATE, perms, 0);
    } break;

    case ">>": {
      stream = Cc["@mozilla.org/network/file-output-stream;1"].
        createInstance(Ci.nsIFileOutputStream);
      stream.init(file, MODE_WRONLY | MODE_CREATE | MODE_APPEND, perms, 0);
    } break;

    default:
      throw "Illegal mode to open(): " + mode;
    }

    return [stream, file];
  },

  readStream: function Weave_readStream(fis) {
    let data = "";
    while (fis.available()) {
      let ret = {};
      fis.readLine(ret);
      data += ret.value;
    }
    return data;
  },

  




  EventListener: function Weave_EventListener(handler, eventName) {
    this._handler = handler;
    this._eventName = eventName;
    this._log = Log4Moz.Service.getLogger("Service.EventHandler");
  }
};

Utils.EventListener.prototype = {
  QueryInterface: XPCOMUtils.generateQI([Ci.nsITimerCallback, Ci.nsISupports]),

  
  handleEvent: function EL_handleEvent(event) {
    this._log.trace("Handling event " + this._eventName);
    this._handler(event);
  },

  
  notify: function EL_notify(timer) {
    this._log.trace("Timer fired");
    this._handler(timer);
  }
}
