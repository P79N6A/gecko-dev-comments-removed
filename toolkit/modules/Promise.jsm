





"use strict";

this.EXPORTED_SYMBOLS = [
  "Promise"
];



















































































const Cc = Components.classes;
const Ci = Components.interfaces;
const Cu = Components.utils;
const Cr = Components.results;

Cu.import("resource://gre/modules/Services.jsm");
Cu.import("resource://gre/modules/XPCOMUtils.jsm");

const STATUS_PENDING = 0;
const STATUS_RESOLVED = 1;
const STATUS_REJECTED = 2;






const salt = Math.floor(Math.random() * 100);
const Name = (n) => "{private:" + n + ":" + salt + "}";
const N_STATUS = Name("status");
const N_VALUE = Name("value");
const N_HANDLERS = Name("handlers");
const N_WITNESS = Name("witness");

































XPCOMUtils.defineLazyServiceGetter(this, "FinalizationWitnessService",
                                   "@mozilla.org/toolkit/finalizationwitness;1",
                                   "nsIFinalizationWitnessService");

let PendingErrors = {
  _counter: 0,
  _map: new Map(),
  register: function(error) {
    let id = "pending-error-" + (this._counter++);
    
    
    
    
    
    
    
    
    
    
    
    
    let value = {
      date: new Date(),
      message: "" + error,
      fileName: null,
      stack: null,
      lineNumber: null
    };
    try { 
      if (typeof error == "object" && error) {
        for (let k of ["fileName", "stack", "lineNumber"]) {
          try { 
            let v = error[k];
            value[k] = v ? ("" + v):null;
          } catch (ex) {
            
          }
        }
      }
    } catch (ex) {
      
    }
    this._map.set(id, value);
    return id;
  },
  extract: function(id) {
    let value = this._map.get(id);
    this._map.delete(id);
    return value;
  },
  unregister: function(id) {
    this._map.delete(id);
  }
};


Services.obs.addObserver(function observe(aSubject, aTopic, aValue) {
  let error = PendingErrors.extract(aValue);
  let {message, date, fileName, stack, lineNumber} = error;
  let error = Cc['@mozilla.org/scripterror;1'].createInstance(Ci.nsIScriptError);
  if (!error || !Services.console) {
    
    dump("*************************\n");
    dump("A promise chain failed to handle a rejection\n\n");
    dump("On: " + date + "\n");
    dump("Full message: " + message + "\n");
    dump("See https://developer.mozilla.org/Mozilla/JavaScript_code_modules/Promise.jsm/Promise\n");
    dump("Full stack: " + (stack||"not available") + "\n");
    dump("*************************\n");
    return;
  }
  if (stack) {
    message += " at " + stack;
  }
  error.init(
             "A promise chain failed to handle a rejection: on " +
               date + ", " + message,
              fileName,
              lineNumber?("" + lineNumber):0,
              lineNumber || 0,
              0,
              Ci.nsIScriptError.errorFlag,
              "chrome javascript");
  Services.console.logMessage(error);
}, "promise-finalization-witness", false);





const ERRORS_TO_REPORT = ["EvalError", "RangeError", "ReferenceError", "TypeError"];







this.Promise = Object.freeze({
  






  defer: function ()
  {
    return new Deferred();
  },

  











  resolve: function (aValue)
  {
    let promise = new PromiseImpl();
    PromiseWalker.completePromise(promise, STATUS_RESOLVED, aValue);
    return promise;
  },

  













  reject: function (aReason)
  {
    let promise = new PromiseImpl();
    PromiseWalker.completePromise(promise, STATUS_REJECTED, aReason);
    return promise;
  },

  















  all: function (aValues)
  {
    if (!Array.isArray(aValues)) {
      throw new Error("Promise.all() expects an array of promises or values.");
    }

    if (!aValues.length) {
      return Promise.resolve([]);
    }

    let countdown = aValues.length;
    let deferred = Promise.defer();
    let resolutionValues = new Array(countdown);

    function checkForCompletion(aValue, aIndex) {
      resolutionValues[aIndex] = aValue;

      if (--countdown === 0) {
        deferred.resolve(resolutionValues);
      }
    }

    for (let i = 0; i < aValues.length; i++) {
      let index = i;
      let value = aValues[i];
      let resolve = val => checkForCompletion(val, index);

      if (value && typeof(value.then) == "function") {
        value.then(resolve, deferred.reject);
      } else {
        
        resolve(value);
      }
    }

    return deferred.promise;
  },
});












this.PromiseWalker = {
  




  handlers: [],

  










  completePromise: function (aPromise, aStatus, aValue)
  {
    
    if (aPromise[N_STATUS] != STATUS_PENDING) {
      return;
    }

    
    
    if (aStatus == STATUS_RESOLVED && aValue &&
        typeof(aValue.then) == "function") {
      aValue.then(this.completePromise.bind(this, aPromise, STATUS_RESOLVED),
                  this.completePromise.bind(this, aPromise, STATUS_REJECTED));
      return;
    }

    
    aPromise[N_STATUS] = aStatus;
    aPromise[N_VALUE] = aValue;
    if (aPromise[N_HANDLERS].length > 0) {
      this.schedulePromise(aPromise);
    } else if (aStatus == STATUS_REJECTED) {
      
      
      let id = PendingErrors.register(aValue);
      let witness =
          FinalizationWitnessService.make("promise-finalization-witness", id);
      aPromise[N_WITNESS] = [id, witness];
    }
  },

  


  scheduleWalkerLoop: function()
  {
    this.walkerLoopScheduled = true;
    Services.tm.currentThread.dispatch(this.walkerLoop,
                                       Ci.nsIThread.DISPATCH_NORMAL);
  },

  







  schedulePromise: function (aPromise)
  {
    
    for (let handler of aPromise[N_HANDLERS]) {
      this.handlers.push(handler);
    }
    aPromise[N_HANDLERS].length = 0;

    
    if (!this.walkerLoopScheduled) {
      this.scheduleWalkerLoop();
    }
  },

  



  walkerLoopScheduled: false,

  






  walkerLoop: function ()
  {
    
    
    
    
    
    
    
    
    
    
    if (this.handlers.length > 1) {
      this.scheduleWalkerLoop();
    } else {
      this.walkerLoopScheduled = false;
    }

    
    while (this.handlers.length > 0) {
      this.handlers.shift().process();
    }
  },
};


PromiseWalker.walkerLoop = PromiseWalker.walkerLoop.bind(PromiseWalker);








function Deferred()
{
  this.promise = new PromiseImpl();
  this.resolve = this.resolve.bind(this);
  this.reject = this.reject.bind(this);

  Object.freeze(this);
}

Deferred.prototype = {
  


  promise: null,

  

















  resolve: function (aValue) {
    PromiseWalker.completePromise(this.promise, STATUS_RESOLVED, aValue);
  },

  
















  reject: function (aReason) {
    PromiseWalker.completePromise(this.promise, STATUS_REJECTED, aReason);
  },
};








function PromiseImpl()
{
  



  Object.defineProperty(this, N_STATUS, { value: STATUS_PENDING,
                                          writable: true });

  






  Object.defineProperty(this, N_VALUE, { writable: true });

  



  Object.defineProperty(this, N_HANDLERS, { value: [] });

  






  Object.defineProperty(this, N_WITNESS, { writable: true });

  Object.seal(this);
}

PromiseImpl.prototype = {
  










































  then: function (aOnResolve, aOnReject)
  {
    let handler = new Handler(this, aOnResolve, aOnReject);
    this[N_HANDLERS].push(handler);

    
    
    if (this[N_STATUS] != STATUS_PENDING) {

      
      if (this[N_WITNESS] != null) {
        let [id, witness] = this[N_WITNESS];
        this[N_WITNESS] = null;
        witness.forget();
        PendingErrors.unregister(id);
      }

      PromiseWalker.schedulePromise(this);
    }

    return handler.nextPromise;
  },
};







function Handler(aThisPromise, aOnResolve, aOnReject)
{
  this.thisPromise = aThisPromise;
  this.onResolve = aOnResolve;
  this.onReject = aOnReject;
  this.nextPromise = new PromiseImpl();
}

Handler.prototype = {
  


  thisPromise: null,

  


  onResolve: null,

  


  onReject: null,

  


  nextPromise: null,

  



  process: function()
  {
    
    let nextStatus = this.thisPromise[N_STATUS];
    let nextValue = this.thisPromise[N_VALUE];

    try {
      
      
      
      if (nextStatus == STATUS_RESOLVED) {
        if (typeof(this.onResolve) == "function") {
          nextValue = this.onResolve(nextValue);
        }
      } else if (typeof(this.onReject) == "function") {
        nextValue = this.onReject(nextValue);
        nextStatus = STATUS_RESOLVED;
      }
    } catch (ex) {

      

      if (ex && typeof ex == "object" && "name" in ex &&
          ERRORS_TO_REPORT.indexOf(ex.name) != -1) {

        
        
        
        
        

        dump("*************************\n");
        dump("A coding exception was thrown in a Promise " +
             ((nextStatus == STATUS_RESOLVED) ? "resolution":"rejection") +
             " callback.\n\n");
        dump("Full message: " + ex + "\n");
        dump("See https://developer.mozilla.org/Mozilla/JavaScript_code_modules/Promise.jsm/Promise\n");
        dump("Full stack: " + (("stack" in ex)?ex.stack:"not available") + "\n");
        dump("*************************\n");

      }

      
      nextStatus = STATUS_REJECTED;
      nextValue = ex;
    }

    
    PromiseWalker.completePromise(this.nextPromise, nextStatus, nextValue);
  },
};
