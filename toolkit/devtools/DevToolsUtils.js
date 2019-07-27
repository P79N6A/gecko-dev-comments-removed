



"use strict";



var { Ci, Cu, Cc, components } = require("chrome");
var Services = require("Services");
var promise = require("promise");




exports.safeErrorString = function safeErrorString(aError) {
  try {
    let errorString = aError.toString();
    if (typeof errorString == "string") {
      
      
      try {
        if (aError.stack) {
          let stack = aError.stack.toString();
          if (typeof stack == "string") {
            errorString += "\nStack: " + stack;
          }
        }
      } catch (ee) { }

      
      
      if (typeof aError.lineNumber == "number" && typeof aError.columnNumber == "number") {
        errorString += "Line: " + aError.lineNumber + ", column: " + aError.columnNumber;
      }

      return errorString;
    }
  } catch (ee) { }

  
  return Object.prototype.toString.call(aError);
}




exports.reportException = function reportException(aWho, aException) {
  let msg = aWho + " threw an exception: " + exports.safeErrorString(aException);

  dump(msg + "\n");

  if (Cu && Cu.reportError) {
    




    Cu.reportError(msg);
  }
}















exports.makeInfallible = function makeInfallible(aHandler, aName) {
  if (!aName)
    aName = aHandler.name;

  return function () {
    try {
      return aHandler.apply(this, arguments);
    } catch (ex) {
      let who = "Handler function";
      if (aName) {
        who += " " + aName;
      }
      return exports.reportException(who, ex);
    }
  }
}










exports.zip = function zip(a, b) {
  if (!b) {
    return a;
  }
  if (!a) {
    return b;
  }
  const pairs = [];
  for (let i = 0, aLength = a.length, bLength = b.length;
       i < aLength || i < bLength;
       i++) {
    pairs.push([a[i], b[i]]);
  }
  return pairs;
};




exports.executeSoon = function executeSoon(aFn) {
  if (isWorker) {
    setImmediate(aFn);
  } else {
    Services.tm.mainThread.dispatch({
      run: exports.makeInfallible(aFn)
    }, Ci.nsIThread.DISPATCH_NORMAL);
  }
};







exports.waitForTick = function waitForTick() {
  let deferred = promise.defer();
  exports.executeSoon(deferred.resolve);
  return deferred.promise;
};









exports.waitForTime = function waitForTime(aDelay) {
  let deferred = promise.defer();
  require("Timer").setTimeout(deferred.resolve, aDelay);
  return deferred.promise;
};
















exports.yieldingEach = function yieldingEach(aArray, aFn) {
  const deferred = promise.defer();

  let i = 0;
  let len = aArray.length;
  let outstanding = [deferred.promise];

  (function loop() {
    const start = Date.now();

    while (i < len) {
      
      
      
      
      if (Date.now() - start > 16) {
        exports.executeSoon(loop);
        return;
      }

      try {
        outstanding.push(aFn(aArray[i], i++));
      } catch (e) {
        deferred.reject(e);
        return;
      }
    }

    deferred.resolve();
  }());

  return promise.all(outstanding);
}














exports.defineLazyPrototypeGetter =
function defineLazyPrototypeGetter(aObject, aKey, aCallback) {
  Object.defineProperty(aObject, aKey, {
    configurable: true,
    get: function() {
      const value = aCallback.call(this);

      Object.defineProperty(this, aKey, {
        configurable: true,
        writable: true,
        value: value
      });

      return value;
    }
  });
}











exports.getProperty = function getProperty(aObj, aKey) {
  let root = aObj;
  try {
    do {
      const desc = aObj.getOwnPropertyDescriptor(aKey);
      if (desc) {
        if ("value" in desc) {
          return desc.value;
        }
        
        return exports.hasSafeGetter(desc) ? desc.get.call(root).return : undefined;
      }
      aObj = aObj.proto;
    } while (aObj);
  } catch (e) {
    
    exports.reportException("getProperty", e);
  }
  return undefined;
};









exports.hasSafeGetter = function hasSafeGetter(aDesc) {
  let fn = aDesc.get;
  return fn && fn.callable && fn.class == "Function" && fn.script === undefined;
};













exports.isSafeJSObject = function isSafeJSObject(aObj) {
  
  
  if (isWorker) {
    return false;
  }

  if (Cu.getGlobalForObject(aObj) ==
      Cu.getGlobalForObject(exports.isSafeJSObject)) {
    return true; 
  }

  let principal = Cu.getObjectPrincipal(aObj);
  if (Services.scriptSecurityManager.isSystemPrincipal(principal)) {
    return true; 
  }

  return Cu.isXrayWrapper(aObj);
};

exports.dumpn = function dumpn(str) {
  if (exports.dumpn.wantLogging) {
    dump("DBG-SERVER: " + str + "\n");
  }
}



exports.dumpn.wantLogging = false;




exports.dumpv = function(msg) {
  if (exports.dumpv.wantVerbose) {
    exports.dumpn(msg);
  }
};



exports.dumpv.wantVerbose = false;

exports.dbg_assert = function dbg_assert(cond, e) {
  if (!cond) {
    return e;
  }
};












exports.update = function update(aTarget, ...aArgs) {
  for (let attrs of aArgs) {
    for (let key in attrs) {
      let desc = Object.getOwnPropertyDescriptor(attrs, key);

      if (desc) {
        Object.defineProperty(aTarget, key, desc);
      }
    }
  }

  return aTarget;
}







exports.values = function values(aObject) {
  return Object.keys(aObject).map(k => aObject[k]);
}












exports.defineLazyGetter = function defineLazyGetter(aObject, aName, aLambda) {
  Object.defineProperty(aObject, aName, {
    get: function () {
      delete aObject[aName];
      return aObject[aName] = aLambda.apply(aObject);
    },
    configurable: true,
    enumerable: true
  });
};















exports.defineLazyModuleGetter = function defineLazyModuleGetter(aObject, aName,
                                                                 aResource,
                                                                 aSymbol)
{
  this.defineLazyGetter(aObject, aName, function XPCU_moduleLambda() {
    var temp = {};
    Cu.import(aResource, temp);
    return temp[aSymbol || aName];
  });
};

exports.defineLazyGetter(this, "NetUtil", () => {
  return Cu.import("resource://gre/modules/NetUtil.jsm", {}).NetUtil;
});

exports.defineLazyGetter(this, "OS", () => {
  return Cu.import("resource://gre/modules/osfile.jsm", {}).OS;
});

exports.defineLazyGetter(this, "TextDecoder", () => {
  return Cu.import("resource://gre/modules/osfile.jsm", {}).TextDecoder;
});
























function mainThreadFetch(aURL, aOptions={ loadFromCache: true,
                                          policy: Ci.nsIContentPolicy.TYPE_OTHER,
                                          window: null,
                                          charset: null }) {
  
  let url = aURL.split(" -> ").pop();
  let channel;
  try {
    channel = newChannelForURL(url, aOptions);
  } catch (ex) {
    return promise.reject(ex);
  }

  
  channel.loadFlags = aOptions.loadFromCache
    ? channel.LOAD_FROM_CACHE
    : channel.LOAD_BYPASS_CACHE;

  if (aOptions.window) {
    
    channel.loadGroup = aOptions.window.QueryInterface(Ci.nsIInterfaceRequestor)
                          .getInterface(Ci.nsIWebNavigation)
                          .QueryInterface(Ci.nsIDocumentLoader)
                          .loadGroup;
  }

  let deferred = promise.defer();
  let onResponse = (stream, status, request) => {
    if (!components.isSuccessCode(status)) {
      deferred.reject(new Error(`Failed to fetch ${url}. Code ${status}.`));
      return;
    }

    try {
      let charset = channel.contentCharset || aOptions.charset || "UTF-8";

      
      let available = stream.available();
      let source = NetUtil.readInputStreamToString(stream, available, {charset});
      stream.close();

      deferred.resolve({
        content: source,
        contentType: request.contentType
      });
    } catch (ex) {
      let uri = request.originalURI;
      if (ex.name === "NS_BASE_STREAM_CLOSED" && uri instanceof Ci.nsIFileURL) {
        
        
        

        uri.QueryInterface(Ci.nsIFileURL);
        let result = OS.File.read(uri.file.path).then(bytes => {
          
          let decoder = new TextDecoder();
          let content = decoder.decode(bytes);

          
          
          return {
            content,
            contentType: "text/plain"
          };
        });

        deferred.resolve(result);
      } else {
        deferred.reject(ex);
      }
    }
  };

  
  try {
    NetUtil.asyncFetch(channel, onResponse);
  } catch (ex) {
    return promise.reject(ex);
  }

  return deferred.promise;
}








function newChannelForURL(url, { policy }) {
  let channelOptions = {
    contentPolicyType: policy,
    loadUsingSystemPrincipal: true,
    uri: url
  };

  try {
    return NetUtil.newChannel(channelOptions);
  } catch (e) {
    
    
    
    channelOptions.uri = "file://" + url;

    return NetUtil.newChannel(channelOptions);
  }
}



if (!this.isWorker) {
  exports.fetch = mainThreadFetch;
} else {
  
  
  
  exports.fetch = function (url, options) {
    return rpc("fetch", url, options);
  }
}



















exports.settleAll = values => {
  if (values === null || typeof(values[Symbol.iterator]) != "function") {
    throw new Error("settleAll() expects an iterable.");
  }

  let deferred = promise.defer();

  values = Array.isArray(values) ? values : [...values];
  let countdown = values.length;
  let resolutionValues = new Array(countdown);
  let rejectionValue;
  let rejectionOccurred = false;

  if (!countdown) {
    deferred.resolve(resolutionValues);
    return deferred.promise;
  }

  function checkForCompletion() {
    if (--countdown > 0) {
      return;
    }
    if (!rejectionOccurred) {
      deferred.resolve(resolutionValues);
    } else {
      deferred.reject(rejectionValue);
    }
  }

  for (let i = 0; i < values.length; i++) {
    let index = i;
    let value = values[i];
    let resolver = result => {
      resolutionValues[index] = result;
      checkForCompletion();
    };
    let rejecter = error => {
      if (!rejectionOccurred) {
        rejectionValue = error;
        rejectionOccurred = true;
      }
      checkForCompletion();
    };

    if (value && typeof(value.then) == "function") {
      value.then(resolver, rejecter);
    } else {
      
      resolver(value);
    }
  }

  return deferred.promise;
};
