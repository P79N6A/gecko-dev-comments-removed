



































const EXPORTED_SYMBOLS = ['Wrap'];

const Cc = Components.classes;
const Ci = Components.interfaces;
const Cr = Components.results;
const Cu = Components.utils;

Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://weave/log4moz.js");
Cu.import("resource://weave/async.js");
Cu.import("resource://weave/util.js");
Cu.import("resource://weave/faultTolerance.js");

Function.prototype.async = Async.sugar;









let Wrap = {

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  notify: function Weave_notify(name, payload, method ) {
    let savedName = name;
    let savedPayload = payload;
    let savedMethod = method;
    let savedArgs = Array.prototype.slice.call(arguments, 3);

    return function WeaveNotifyWrapper() {
      let self = yield;
      let ret;
      let args = Array.prototype.slice.call(arguments);

      try {
        this._log.debug("Event: " + this._osPrefix + savedName + ":start");
        this._os.notifyObservers(null, this._osPrefix + savedName + ":start", savedPayload);
        this._os.notifyObservers(null, this._osPrefix + "global:start", savedPayload);

        args = savedArgs.concat(args);
        args.unshift(this, savedMethod, self.cb);
        Async.run.apply(Async, args);
        ret = yield;

        this._log.debug("Event: " + this._osPrefix + savedName + ":success");
        this._os.notifyObservers(null, this._osPrefix + savedName + ":success", savedPayload);
        this._os.notifyObservers(null, this._osPrefix + "global:success", savedPayload);

      } catch (e) {
        this._log.debug("Event: " + this._osPrefix + savedName + ":error");
        this._os.notifyObservers(null, this._osPrefix + savedName + ":error", savedPayload);
        this._os.notifyObservers(null, this._osPrefix + "global:error", savedPayload);
	throw e;
      }

      self.done(ret);
    };
  },

  
  
  localLock: function WeaveSync_localLock(method ) {
    let savedMethod = method;
    let savedArgs = Array.prototype.slice.call(arguments, 1);

    return function WeaveLocalLockWrapper() {
      let self = yield;
      let ret;
      let args = Array.prototype.slice.call(arguments);

      ret = this.lock();
      if (!ret)
        throw "Could not acquire lock";

      this._os.notifyObservers(null,
                               this._osPrefix + "local-lock:acquired", "");

      try {
        args = savedArgs.concat(args);
        args.unshift(this, savedMethod, self.cb);
        ret = yield Async.run.apply(Async, args);

      } catch (e) {
        throw e;

      } finally {
        this.unlock();
        this._os.notifyObservers(null,
                                 this._osPrefix + "local-lock:released", "");
      }

      self.done(ret);
    };
  },

  
  
  
  catchAll: function WeaveSync_catchAll(method ) {
    let savedMethod = method;
    let savedArgs = Array.prototype.slice.call(arguments, 1);

    return function WeaveCatchAllWrapper() {
      let self = yield;
      let ret;
      let args = Array.prototype.slice.call(arguments);

      try {
        args = savedArgs.concat(args);
        args.unshift(this, savedMethod, self.cb);
        ret = yield Async.run.apply(Async, args);

      } catch (e) {
	this._log.debug("Caught exception: " + Utils.exceptionStr(e));
      }
      self.done(ret);
    };
  }
}
