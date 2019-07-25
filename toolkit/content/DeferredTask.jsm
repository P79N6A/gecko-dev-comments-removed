



const EXPORTED_SYMBOLS = ["DeferredTask"];

const Cu = Components.utils;
const Cc = Components.classes;
const Ci = Components.interfaces;
















function DeferredTask(aCallback, aDelay) {
  this._callback = function onCallback() {
    this._timer = null;
    try {
      aCallback();
    } catch(e) {
      Cu.reportError(e);
    }
  }.bind(this);
  this._delay = aDelay;
}

DeferredTask.prototype = {
  
  _callback: null,
  
  _delay: null,
  
  _timer: null,

  



  get isPending() {
    return (this._timer != null);
  },

  


  start: function DeferredTask_start() {
    if (this._timer) {
      this._timer.cancel();
    }
    this._timer = Cc["@mozilla.org/timer;1"].createInstance(Ci.nsITimer);
    this._timer.initWithCallback(
      this._callback, this._delay, Ci.nsITimer.TYPE_ONE_SHOT);
  },

  


  flush: function DeferredTask_flush() {
    if (this._timer) {
      this.cancel();
      this._callback();
    }
  },

  


  cancel: function DeferredTask_cancel() {
    if (this._timer) {
      this._timer.cancel();
      this._timer = null;
    }
  }
};
