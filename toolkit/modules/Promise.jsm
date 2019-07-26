





"use strict";

this.EXPORTED_SYMBOLS = [
  "Promise"
];



















































































const Cc = Components.classes;
const Ci = Components.interfaces;
const Cu = Components.utils;
const Cr = Components.results;

Cu.import("resource://gre/modules/Services.jsm");

const STATUS_PENDING = 0;
const STATUS_RESOLVED = 1;
const STATUS_REJECTED = 2;






const salt = Math.floor(Math.random() * 100);
const Name = (n) => "{private:" + n + ":" + salt + "}";
const N_STATUS = Name("status");
const N_VALUE = Name("value");
const N_HANDLERS = Name("handlers");







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
    }
  },

  







  schedulePromise: function (aPromise)
  {
    
    for (let handler of aPromise[N_HANDLERS]) {
      this.handlers.push(handler);
    }
    aPromise[N_HANDLERS].length = 0;

    
    if (!this.walkerLoopScheduled) {
      this.walkerLoopScheduled = true;
      Services.tm.currentThread.dispatch(this.walkerLoop,
                                         Ci.nsIThread.DISPATCH_NORMAL);
    }
  },

  



  walkerLoopScheduled: false,

  






  walkerLoop: function ()
  {
    
    
    
    
    
    
    
    
    this.walkerLoopScheduled = false;

    
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

  Object.seal(this);
}

PromiseImpl.prototype = {
  










































  then: function (aOnResolve, aOnReject)
  {
    let handler = new Handler(this, aOnResolve, aOnReject);
    this[N_HANDLERS].push(handler);

    
    
    if (this[N_STATUS] != STATUS_PENDING) {
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
      
      nextStatus = STATUS_REJECTED;
      nextValue = ex;
    }

    
    PromiseWalker.completePromise(this.nextPromise, nextStatus, nextValue);
  },
};
