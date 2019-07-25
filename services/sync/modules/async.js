




































const EXPORTED_SYMBOLS = ['Async'];

const Cc = Components.classes;
const Ci = Components.interfaces;
const Cr = Components.results;
const Cu = Components.utils;


const CB_READY = {};
const CB_COMPLETE = {};
const CB_FAIL = {};

const REASON_ERROR = Ci.mozIStorageStatementCallback.REASON_ERROR;

Cu.import("resource://services-sync/util.js");




let Async = {

  





  



  makeSyncCallback: function makeSyncCallback() {
    
    let onComplete = function onComplete(data) {
      onComplete.state = CB_COMPLETE;
      onComplete.value = data;
    };

    
    onComplete.state = CB_READY;
    onComplete.value = null;

    
    onComplete.throw = function onComplete_throw(data) {
      onComplete.state = CB_FAIL;
      onComplete.value = data;

      
      throw data;
    };

    return onComplete;
  },

  


  waitForSyncCallback: function waitForSyncCallback(callback) {
    
    let thread = Cc["@mozilla.org/thread-manager;1"].getService().currentThread;

    
    while (Utils.checkAppReady() && callback.state == CB_READY) {
      thread.processNextEvent(true);
    }

    
    let state = callback.state;
    callback.state = CB_READY;

    
    if (state == CB_FAIL) {
      throw callback.value;
    }

    
    return callback.value;
  },

  



  makeSpinningCallback: function makeSpinningCallback() {
    let cb = Async.makeSyncCallback();
    function callback(error, ret) {
      if (error)
        cb.throw(error);
      cb(ret);
    }
    callback.wait = function() Async.waitForSyncCallback(cb);
    return callback;
  },

  
  
  
  _storageCallbackPrototype: {
    results: null,

    
    names: null,
    syncCb: null,

    handleResult: function handleResult(results) {
      if (!this.names) {
        return;
      }
      if (!this.results) {
        this.results = [];
      }
      let row;
      while ((row = results.getNextRow()) != null) {
        let item = {};
        for each (let name in this.names) {
          item[name] = row.getResultByName(name);
        }
        this.results.push(item);
      }
    },
    handleError: function handleError(error) {
      this.syncCb.throw(error);
    },
    handleCompletion: function handleCompletion(reason) {

      
      
      
      if (reason == REASON_ERROR)
        return;

      
      
      if (this.names && !this.results) {
        this.results = [];
      }
      this.syncCb(this.results);
    }
  },

  querySpinningly: function querySpinningly(query, names) {
    
    let storageCallback = {names: names,
                           syncCb: Async.makeSyncCallback()};
    storageCallback.__proto__ = Async._storageCallbackPrototype;
    query.executeAsync(storageCallback);
    return Async.waitForSyncCallback(storageCallback.syncCb);
  },
};
