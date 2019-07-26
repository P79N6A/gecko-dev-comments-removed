





"use strict";

let EXPORTED_SYMBOLS = [
  "Task"
];

















































































const Cc = Components.classes;
const Ci = Components.interfaces;
const Cu = Components.utils;
const Cr = Components.results;

Cu.import("resource://gre/modules/commonjs/promise/core.js");







const Task = {
  



















  spawn: function Task_spawn(aTask) {
    if (aTask && typeof(aTask) == "function") {
      
      aTask = aTask();
    }

    if (aTask && typeof(aTask.send) == "function") {
      
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
  this._run(true);
}

TaskImpl.prototype = {
  



  deferred: null,

  


  _iterator: null,

  











  _run: function TaskImpl_run(aSendResolved, aSendValue) {
    try {
      let yielded = aSendResolved ? this._iterator.send(aSendValue)
                                  : this._iterator.throw(aSendValue);

      
      
      
      if (yielded && typeof(yielded.send) == "function") {
        yielded = Task.spawn(yielded);
      }

      if (yielded && typeof(yielded.then) == "function") {
        
        
        
        yielded.then(this._run.bind(this, true),
                     this._run.bind(this, false));
      } else {
        
        
        this._run(true, yielded);
      }

    } catch (ex if ex instanceof Task.Result) {
      
      
      this.deferred.resolve(ex.value);
    } catch (ex if ex instanceof StopIteration) {
      
      this.deferred.resolve();
    } catch (ex) {
      
      this.deferred.reject(ex);
    }
  }
};
