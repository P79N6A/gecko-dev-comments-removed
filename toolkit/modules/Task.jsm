





"use strict";

this.EXPORTED_SYMBOLS = [
  "Task"
];













































































const Cc = Components.classes;
const Ci = Components.interfaces;
const Cu = Components.utils;
const Cr = Components.results;

Cu.import("resource://gre/modules/commonjs/sdk/core/promise.js");



const ERRORS_TO_REPORT = ["EvalError", "RangeError", "ReferenceError", "TypeError"];








function isGenerator(aValue) {
  return Object.prototype.toString.call(aValue) == "[object Generator]";
}







this.Task = {
  



















  spawn: function Task_spawn(aTask) {
    if (aTask && typeof(aTask) == "function") {
      try {
        
        aTask = aTask();
      } catch (ex if ex instanceof Task.Result) {
        return Promise.resolve(ex.value);
      } catch (ex) {
        return Promise.reject(ex);
      }
    }

    if (isGenerator(aTask)) {
      
      return new TaskImpl(aTask).deferred.promise;
    }

    
    return Promise.resolve(aTask);
  },

  






  Result: function Task_Result(aValue) {
    this.value = aValue;
  }
};








function TaskImpl(iterator) {
  this.deferred = Promise.defer();
  this._iterator = iterator;
  this._isStarGenerator = !("send" in iterator);
  this._run(true);
}

TaskImpl.prototype = {
  



  deferred: null,

  


  _iterator: null,

  


  _isStarGenerator: false,

  











  _run: function TaskImpl_run(aSendResolved, aSendValue) {
    if (this._isStarGenerator) {
      try {
        let result = aSendResolved ? this._iterator.next(aSendValue)
                                   : this._iterator.throw(aSendValue);

        if (result.done) {
          
          this.deferred.resolve(result.value);
        } else {
          
          this._handleResultValue(result.value);
        }
      } catch (ex) {
        
        this._handleException(ex);
      }
    } else {
      try {
        let yielded = aSendResolved ? this._iterator.send(aSendValue)
                                    : this._iterator.throw(aSendValue);
        this._handleResultValue(yielded);
      } catch (ex if ex instanceof Task.Result) {
        
        
        this.deferred.resolve(ex.value);
      } catch (ex if ex instanceof StopIteration) {
        
        this.deferred.resolve();
      } catch (ex) {
        
        this._handleException(ex);
      }
    }
  },

  





  _handleResultValue: function TaskImpl_handleResultValue(aValue) {
    
    
    
    if (isGenerator(aValue)) {
      aValue = Task.spawn(aValue);
    }

    if (aValue && typeof(aValue.then) == "function") {
      
      
      
      aValue.then(this._run.bind(this, true),
                  this._run.bind(this, false));
    } else {
      
      
      this._run(true, aValue);
    }
  },

  





  _handleException: function TaskImpl_handleException(aException) {
    if (aException && typeof aException == "object" && "name" in aException &&
        ERRORS_TO_REPORT.indexOf(aException.name) != -1) {

      
      
      
      
      

      let stack = ("stack" in aException) ? aException.stack : "not available";
      dump("*************************\n");
      dump("A coding exception was thrown and uncaught in a Task.\n\n");
      dump("Full message: " + aException + "\n");
      dump("Full stack: " + stack + "\n");
      dump("*************************\n");
    }

    this.deferred.reject(aException);
  }
};
