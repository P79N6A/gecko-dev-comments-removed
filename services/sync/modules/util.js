



































const EXPORTED_SYMBOLS = ['deepEquals', 'makeFile', 'makeURI', 'xpath',
			  'bind2', 'generatorAsync', 'generatorDone', 'EventListener'];

const Cc = Components.classes;
const Ci = Components.interfaces;
const Cr = Components.results;
const Cu = Components.utils;

Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://weave/constants.js");
Cu.import("resource://weave/log4moz.js");





function deepEquals(a, b) {
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
      if (!deepEquals(a[key], b[key]))
        return false;
    } else {
      if (a[key] != b[key])
        return false;
    }
  }
  return true;
}

function makeFile(path) {
  var file = Cc["@mozilla.org/file/local;1"].createInstance(Ci.nsILocalFile);
  file.initWithPath(path);
  return file;
}

function makeURI(URIString) {
  if (URIString === null || URIString == "")
    return null;
  let ioservice = Cc["@mozilla.org/network/io-service;1"].
    getService(Ci.nsIIOService);
  return ioservice.newURI(URIString, null, null);
}

function xpath(xmlDoc, xpathString) {
  let root = xmlDoc.ownerDocument == null ?
    xmlDoc.documentElement : xmlDoc.ownerDocument.documentElement
  let nsResolver = xmlDoc.createNSResolver(root);

  return xmlDoc.evaluate(xpathString, xmlDoc, nsResolver,
                         Ci.nsIDOMXPathResult.ANY_TYPE, null);
}

function bind2(object, method) {
  return function innerBind() { return method.apply(object, arguments); }
}













function generatorAsync(self, extra_args) {
  try {
    let args = Array.prototype.slice.call(arguments, 1);
    let gen = this.apply(self, args);
    gen.next(); 
    gen.send([gen, function(data) {continueGenerator(gen, data);}]);
    return gen;
  } catch (e) {
    if (e instanceof StopIteration) {
      dump("async warning: generator stopped unexpectedly");
      return null;
    } else {
      dump("Exception caught: " + e.message);
    }
  }
}

function continueGenerator(generator, data) {
  try { generator.send(data); }
  catch (e) {
    if (e instanceof StopIteration)
      dump("continueGenerator warning: generator stopped unexpectedly");
    else
      dump("Exception caught: " + e.message);
  }
}








function generatorDone(object, generator, callback, retval) {
  if (object._timer)
    throw "Called generatorDone when there is a timer already set."

  let cb = bind2(object, function(event) {
    generator.close();
    generator = null;
    object._timer = null;
    if (callback)
      callback(retval);
  });

  object._timer = Cc["@mozilla.org/timer;1"].createInstance(Ci.nsITimer);
  object._timer.initWithCallback(new EventListener(cb),
                                 0, object._timer.TYPE_ONE_SHOT);
}






function EventListener(handler, eventName) {
  this._handler = handler;
  this._eventName = eventName;
  this._log = Log4Moz.Service.getLogger("Service.EventHandler");
}
EventListener.prototype = {
  QueryInterface: XPCOMUtils.generateQI([Ci.nsITimerCallback, Ci.nsISupports]),

  
  handleEvent: function EL_handleEvent(event) {
    this._log.debug("Handling event " + this._eventName);
    this._handler(event);
  },

  
  notify: function EL_notify(timer) {
    this._log.trace("Timer fired");
    this._handler(timer);
  }
};
