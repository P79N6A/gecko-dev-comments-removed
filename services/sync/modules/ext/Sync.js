





































let EXPORTED_SYMBOLS = ["Sync"];

const Cc = Components.classes;
const Ci = Components.interfaces;
const Cr = Components.results;
const Cu = Components.utils;


const CB_READY = {};
const CB_COMPLETE = {};
const CB_FAIL = {};


const SECRET = {};




function checkAppReady() {
  
  let os = Cc["@mozilla.org/observer-service;1"].
    getService(Ci.nsIObserverService);
  os.addObserver({
    observe: function observe() {
      
      checkAppReady = function() {
        throw Components.Exception("App. Quitting", Cr.NS_ERROR_ABORT);
      };
      os.removeObserver(this, "quit-application");
    }
  }, "quit-application", false);

  
  return (checkAppReady = function() true)();
};




function makeCallback() {
  
  let _ = {
    state: CB_READY,
    value: null
  };

  
  let onComplete = function makeCallback_onComplete(data) {
    _.state = CB_COMPLETE;
    _.value = data;
  };

  
  onComplete._ = function onComplete__(secret) secret == SECRET ? _ : {};

  
  onComplete.throw = function onComplete_throw(data) {
    _.state = CB_FAIL;
    _.value = data;

    
    throw data;
  };

  return onComplete;
}















function Sync(func, thisArg, callback) {
  return function syncFunc() {
    
    let thread = Cc["@mozilla.org/thread-manager;1"].getService().currentThread;

    
    let args = Array.slice(arguments);

    let instanceCallback = callback;
    
    if (instanceCallback == null) {
      
      instanceCallback = makeCallback();
      args.unshift(instanceCallback);
    }

    
    func.apply(thisArg, args);

    
    let callbackData = instanceCallback._(SECRET);
    while (checkAppReady() && callbackData.state == CB_READY)
      thread.processNextEvent(true);

    
    let state = callbackData.state;
    callbackData.state = CB_READY;

    
    if (state == CB_FAIL)
      throw callbackData.value;

    
    return callbackData.value;
  };
}











Sync.withCb = function Sync_withCb(func, thisArg) {
  let cb = makeCallback();
  return [Sync(func, thisArg, cb), cb];
};












function setTimeout(func, delay) {
  let timer = Cc["@mozilla.org/timer;1"].createInstance(Ci.nsITimer);
  let callback = {
    notify: function notify() {
      
      timer = null;

      
      func();
    }
  }
  timer.initWithCallback(callback, delay, Ci.nsITimer.TYPE_ONE_SHOT);
}

function sleep(callback, milliseconds) {
  setTimeout(callback, milliseconds);
}















Sync.sleep = Sync(sleep);
