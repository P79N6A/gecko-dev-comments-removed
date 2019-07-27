



"use strict";
loader.lazyRequireGetter(this, "timers",
  "resource://gre/modules/Timer.jsm");
loader.lazyRequireGetter(this, "defer",
  "sdk/core/promise", true);
















function Poller (fn, wait, immediate) {
  this._fn = fn;
  this._wait = wait;
  this._immediate = immediate;
  this._poll = this._poll.bind(this);
  this._preparePoll = this._preparePoll.bind(this);
}
exports.Poller = Poller;







Poller.prototype.isPolling = function pollerIsPolling () {
  return !!this._timer;
};






Poller.prototype.on = function pollerOn () {
  if (this._destroyed) {
    throw Error("Poller cannot be turned on after destruction.");
  }
  if (this._timer) {
    this.off();
  }
  this._immediate ? this._poll() : this._preparePoll();
  return this;
};







Poller.prototype.off = function pollerOff () {
  let { resolve, promise } = defer();
  if (this._timer) {
    timers.clearTimeout(this._timer);
    this._timer = null;
  }

  
  
  if (this._inflight) {
    this._inflight.then(resolve);
  } else {
    resolve();
  }
  return promise;
};





Poller.prototype.destroy = function pollerDestroy () {
  return this.off().then(() => {
    this._destroyed = true;
    this._fn = null
  });
};

Poller.prototype._preparePoll = function pollerPrepare () {
  this._timer = timers.setTimeout(this._poll, this._wait);
};

Poller.prototype._poll = function pollerPoll () {
  let response = this._fn();
  if (response && typeof response.then === "function") {
    
    
    this._inflight = response;
    response.then(() => {
      
      
      if (this._timer) {
        this._preparePoll();
      }
    });
  } else {
    this._preparePoll();
  }
};
