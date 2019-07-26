



"use strict";



let { Promise: promise } = Components.utils.import("resource://gre/modules/commonjs/sdk/core/promise.js", {});
let { Services } = Components.utils.import("resource://gre/modules/Services.jsm", {});




this.safeErrorString = function safeErrorString(aError) {
  try {
    let errorString = aError.toString();
    if (typeof errorString === "string") {
      
      
      try {
        if (aError.stack) {
          let stack = aError.stack.toString();
          if (typeof stack === "string") {
            errorString += "\nStack: " + stack;
          }
        }
      } catch (ee) { }

      return errorString;
    }
  } catch (ee) { }

  return "<failed trying to find error description>";
}




this.reportException = function reportException(aWho, aException) {
  let msg = aWho + " threw an exception: " + safeErrorString(aException);

  dump(msg + "\n");

  if (Components.utils.reportError) {
    




    Components.utils.reportError(msg);
  }
}















this.makeInfallible = function makeInfallible(aHandler, aName) {
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
      reportException(who, ex);
    }
  }
}











this.zip = function zip(a, b) {
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

const executeSoon = aFn => {
  Services.tm.mainThread.dispatch({
    run: this.makeInfallible(aFn)
  }, Components.interfaces.nsIThread.DISPATCH_NORMAL);
};














this.yieldingEach = function yieldingEach(aArray, aFn) {
  const deferred = promise.defer();

  let i = 0;
  let len = aArray.length;

  (function loop() {
    const start = Date.now();

    while (i < len) {
      
      
      
      
      if (Date.now() - start > 16) {
        executeSoon(loop);
        return;
      }

      try {
        aFn(aArray[i++]);
      } catch (e) {
        deferred.reject(e);
        return;
      }
    }

    deferred.resolve();
  }());

  return deferred.promise;
}















this.defineLazyPrototypeGetter =
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











this.getProperty = function getProperty(aObj, aKey) {
  let root = aObj;
  try {
    do {
      const desc = aObj.getOwnPropertyDescriptor(aKey);
      if (desc) {
        if ("value" in desc) {
          return desc.value;
        }
        
        return hasSafeGetter(desc) ? desc.get.call(root).return : undefined;
      }
      aObj = aObj.proto;
    } while (aObj);
  } catch (e) {
    
    reportException("getProperty", e);
  }
  return undefined;
};









this.hasSafeGetter = function hasSafeGetter(aDesc) {
  let fn = aDesc.get;
  return fn && fn.callable && fn.class == "Function" && fn.script === undefined;
};

