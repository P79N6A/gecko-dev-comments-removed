







"use strict";

const Cu = Components.utils;
const Ci = Components.interfaces;
const Cc = Components.classes;
const Cr = Components.results;

let XPCOMUtils = Cu.import("resource://gre/modules/XPCOMUtils.jsm", {}).XPCOMUtils;
XPCOMUtils.defineLazyModuleGetter(this, "AsyncShutdown",
  "resource://gre/modules/AsyncShutdown.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "Promise",
  "resource://gre/modules/Promise.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "Task",
  "resource://gre/modules/Task.jsm");





let PropertyBagConverter = {
  
  toObject: function(bag) {
    if (!(bag instanceof Ci.nsIPropertyBag)) {
      throw new TypeError("Not a property bag");
    }
    let result = {};
    let enumerator = bag.enumerator;
    while (enumerator.hasMoreElements()) {
      let {name, value: property} = enumerator.getNext().QueryInterface(Ci.nsIProperty);
      let value = this.toValue(property);
      result[name] = value;
    }
    return result;
  },
  toValue: function(property) {
    if (typeof property != "object") {
      return property;
    }
    if (Array.isArray(property)) {
      return property.map(this.toValue, this);
    }
    if (property && property instanceof Ci.nsIPropertyBag) {
      return this.toObject(property);
    }
    return property;
  },

  
  fromObject: function(obj) {
    if (obj == null || typeof obj != "object") {
      throw new TypeError("Invalid object: " + obj);
    }
    let bag = Cc["@mozilla.org/hash-property-bag;1"].
      createInstance(Ci.nsIWritablePropertyBag);
    for (let k of Object.keys(obj)) {
      let value = this.fromValue(obj[k]);
      bag.setProperty(k, value);
    }
    return bag;
  },
  fromValue: function(value) {
    if (typeof value == "function") {
      return null; 
    }
    if (Array.isArray(value)) {
      return value.map(this.fromValue, this);
    }
    if (value == null || typeof value != "object") {
      
      return value;
    }
    return this.fromObject(value);
  },
};












function nsAsyncShutdownClient(moduleClient) {
  if (!moduleClient) {
    throw new TypeError("nsAsyncShutdownClient expects one argument");
  }
  this._moduleClient = moduleClient;
  this._byName = new Map();
}
nsAsyncShutdownClient.prototype = {
  _getPromisified: function(xpcomBlocker) {
    let candidate = this._byName.get(xpcomBlocker.name);
    if (!candidate) {
      return null;
    }
    if (candidate.xpcom === xpcomBlocker) {
      return candidate.jsm;
    }
    return null;
  },
  _setPromisified: function(xpcomBlocker, moduleBlocker) {
    let candidate = this._byName.get(xpcomBlocker.name);
    if (!candidate) {
      this._byName.set(xpcomBlocker.name, {xpcom: xpcomBlocker,
                                           jsm: moduleBlocker});
      return;
    }
    if (candidate.xpcom === xpcomBlocker) {
      return;
    }
    throw new Error("We have already registered a distinct blocker with the same name: " + xpcomBlocker.name);
  },
  _deletePromisified: function(xpcomBlocker) {
    let candidate = this._byName.get(xpcomBlocker.name);
    if (!candidate || candidate.xpcom !== xpcomBlocker) {
      return false;
    }
    this._byName.delete(xpcomBlocker.name);
    return true;
  },
  get jsclient() {
    return this._moduleClient;
  },
  get name() {
    return this._moduleClient.name;
  },
  addBlocker: function( xpcomBlocker,
      fileName, lineNumber, stack) {
    
    
    
    
    
    
    
    
    
    
    let moduleBlocker = this._getPromisified(xpcomBlocker);
    if (!moduleBlocker) {
      moduleBlocker = () => new Promise(
        
        
        
        () => xpcomBlocker.blockShutdown(this)
      );

      this._setPromisified(xpcomBlocker, moduleBlocker);
    }

    this._moduleClient.addBlocker(xpcomBlocker.name,
      moduleBlocker,
      {
        fetchState: () => {
          let state = xpcomBlocker.state;
          if (state) {
            return PropertyBagConverter.toValue(state);
          }
          return null;
        },
        filename: fileName,
        lineNumber: lineNumber,
        stack: stack,
      });
  },

  removeBlocker: function(xpcomBlocker) {
    let moduleBlocker = this._getPromisified(xpcomBlocker);
    if (!moduleBlocker) {
      return false;
    }
    this._deletePromisified(xpcomBlocker);
    return this._moduleClient.removeBlocker(moduleBlocker);
  },

  
  QueryInterface :  XPCOMUtils.generateQI([Ci.nsIAsyncShutdownBarrier]),
  classID:          Components.ID("{314e9e96-cc37-4d5c-843b-54709ce11426}"),
};










function nsAsyncShutdownBarrier(moduleBarrier) {
  this._client = new nsAsyncShutdownClient(moduleBarrier.client);
  this._moduleBarrier = moduleBarrier;
};
nsAsyncShutdownBarrier.prototype = {
  get state() {
    return PropertyBagConverter.fromValue(this._moduleBarrier.state);
  },
  get client() {
    return this._client;
  },
  wait: function(onReady) {
    this._moduleBarrier.wait().then(() => {
      onReady.done();
    });
    
  },

  
  QueryInterface :  XPCOMUtils.generateQI([Ci.nsIAsyncShutdownBarrier]),
  classID:          Components.ID("{29a0e8b5-9111-4c09-a0eb-76cd02bf20fa}"),
};

function nsAsyncShutdownService() {
  

  for (let _k of
   ["profileBeforeChange",
    "profileChangeTeardown",
    "sendTelemetry",
    "webWorkersShutdown",
    "xpcomThreadsShutdown"]) {
    let k = _k;
    Object.defineProperty(this, k, {
      configurable: true,
      get: function() {
        delete this[k];
        let result = new nsAsyncShutdownClient(AsyncShutdown[k]);
        Object.defineProperty(this, k, {
          value: result
        });
        return result;
      }
    });
  }

  
  this.wrappedJSObject = {
    _propertyBagConverter: PropertyBagConverter
  };
}
nsAsyncShutdownService.prototype = {
  makeBarrier: function(name) {
    return new nsAsyncShutdownBarrier(new AsyncShutdown.Barrier(name));
  },

  
  QueryInterface :  XPCOMUtils.generateQI([Ci.nsIAsyncShutdownService]),
  classID:          Components.ID("{35c496de-a115-475d-93b5-ffa3f3ae6fe3}"),
};


this.NSGetFactory = XPCOMUtils.generateNSGetFactory([
    nsAsyncShutdownService,
    nsAsyncShutdownBarrier,
    nsAsyncShutdownClient,
]);

