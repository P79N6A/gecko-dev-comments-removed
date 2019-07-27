



"use strict"

this.EXPORTED_SYMBOLS = ["PromiseUtils"];

Components.utils.import("resource://gre/modules/Timer.jsm");

this.PromiseUtils = {
  













  resolveOrTimeout : function(promise, delay, rejection)  {
    
    if (!(promise instanceof Promise)) {
      throw new TypeError("first argument <promise> must be a Promise object");
    }

    
    if (typeof delay != "number" || delay < 0) {
      throw new TypeError("second argument <delay> must be a positive number");
    }

    
    if (rejection && typeof rejection != "function") {
      throw new TypeError("third optional argument <rejection> must be a function");
    }

    return new Promise((resolve, reject) => {
      promise.then(resolve, reject);
      let id = setTimeout(() => {
        try {
          rejection ? reject(rejection()) : reject(new Error("Promise Timeout"));
        } catch(ex) {
          reject(ex);
        }
        clearTimeout(id);
      }, delay);
    });
  }
}