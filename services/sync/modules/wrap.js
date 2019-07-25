



































const EXPORTED_SYMBOLS = ['Wrap'];

const Cc = Components.classes;
const Ci = Components.interfaces;
const Cr = Components.results;
const Cu = Components.utils;

Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://weave/log4moz.js");
Cu.import("resource://weave/dav.js");
Cu.import("resource://weave/async.js");

Function.prototype.async = Async.sugar;









let Wrap = {

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  notify: function Weave_notify(name, method ) {
    let savedName = name;
    let savedMethod = method;
    let savedArgs = Array.prototype.slice.call(arguments, 2);

    return function WeaveNotifyWrapper() {
      let self = yield;
      let ret;
      let args = Array.prototype.slice.call(arguments);

      try {
        this._os.notifyObservers(null, this._osPrefix + savedName + ":start", "");
        this._os.notifyObservers(null, this._osPrefix + "global:start", "");

        args = savedArgs.concat(args);
        args.unshift(this, savedMethod, self.cb);
        Async.run.apply(Async, args);
        ret = yield;

        this._os.notifyObservers(null, this._osPrefix + savedName + ":success", "");
        this._os.notifyObservers(null, this._osPrefix + "global:success", "");

      } catch (e) {
        this._os.notifyObservers(null, this._osPrefix + savedName + ":error", "");
        this._os.notifyObservers(null, this._osPrefix + "global:error", "");
        throw e;
      }

      self.done(ret);
    };
  },

  
  
  lock: function WeaveSync_lock(method ) {
    let savedMethod = method;
    let savedArgs = Array.prototype.slice.call(arguments, 1);

    return function WeaveLockWrapper( ) {
      let self = yield;
      let ret;
      let args = Array.prototype.slice.call(arguments);

      if (!this._loggedIn)
        throw "Could not acquire lock (not logged in)";
      if (DAV.locked)
        throw "Could not acquire lock (lock already held)";

      DAV.lock.async(DAV, self.cb);
      let locked = yield;
      if (!locked)
        throw "Could not acquire lock";

      this._os.notifyObservers(null, this._osPrefix + "lock:acquired", "");

      try {
        args = savedArgs.concat(args);
        args.unshift(this, savedMethod, self.cb);
        Async.run.apply(Async, args);
        ret = yield;

      } catch (e) {
        throw e;

      } finally {
        yield DAV.unlock.async(DAV, self.cb);
        this._os.notifyObservers(null, this._osPrefix + "lock:released", "");
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

      if (DAV.locked)
        throw "Could not acquire lock";
      DAV.allowLock = false;

      this._os.notifyObservers(null,
                               this._osPrefix + "local-lock:acquired", "");

      try {
        args = savedArgs.concat(args);
        args.unshift(this, savedMethod, self.cb);
        Async.run.apply(Async, args);
        ret = yield;

      } catch (e) {
        throw e;

      } finally {
        DAV.allowLock = true;
        this._os.notifyObservers(null,
                                 this._osPrefix + "local-lock:released", "");
      }

      self.done(ret);
    };
  }
}
