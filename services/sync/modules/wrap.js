



































const EXPORTED_SYMBOLS = ['Wrap'];

const Cc = Components.classes;
const Ci = Components.interfaces;
const Cr = Components.results;
const Cu = Components.utils;

Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://weave/ext/Observers.js");
Cu.import("resource://weave/log4moz.js");
Cu.import("resource://weave/async.js");
Cu.import("resource://weave/util.js");
Cu.import("resource://weave/faultTolerance.js");

Function.prototype.async = Async.sugar;









let Wrap = {

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  notify: function WeaveWrap_notify(prefix) {
    return function NotifyWrapMaker(name, subject, method) {
      let savedArgs = Array.prototype.slice.call(arguments, 4);
      return function NotifyWrap() {
        let self = yield;
        let ret;
        let args = Array.prototype.slice.call(arguments);

        try {
          this._log.debug("Event: " + prefix + name + ":start");
          Observers.notify(prefix + name + ":start", subject);

          args = savedArgs.concat(args);
          args.unshift(this, method, self.cb);
          Async.run.apply(Async, args);
          ret = yield;

          this._log.debug("Event: " + prefix + name + ":finish");
          let foo = Observers.notify(prefix + name + ":finish", subject);

        } catch (e) {
          this._log.debug("Event: " + prefix + name + ":error");
          Observers.notify(prefix + name + ":error", subject);
	  throw e;
        }

        self.done(ret);
      };
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
	this._log.debug("\n" + Utils.stackTrace(e));
      }
      self.done(ret);
    };
  }
}
