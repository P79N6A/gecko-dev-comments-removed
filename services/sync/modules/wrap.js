



































const EXPORTED_SYMBOLS = ['Wrap'];

const Cc = Components.classes;
const Ci = Components.interfaces;
const Cr = Components.results;
const Cu = Components.utils;

Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://weave/log4moz.js");
Cu.import("resource://weave/async.js");

Function.prototype.async = Async.sugar;









let Wrap = {

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  notify: function Weave_notify(name, method ) {
    let savedName = name;
    let savedMethod = method;
    let args = Array.prototype.slice.call(arguments, 2);

    return function WeaveNotifyWrapper() {
      let self = yield;
      let ret;

      try {
        this._os.notifyObservers(null, this._osPrefix + savedName + ":start", "");

        args.unshift(this, savedMethod, self.cb);
        Async.run.apply(Async, args);
        ret = yield;

        this._os.notifyObservers(null, this._osPrefix + savedName + ":success", "");

      } catch (e) {
        this._os.notifyObservers(null, this._osPrefix + savedName + ":error", "");
        throw e;
      }

      self.done(ret);
    };
  },

  
  
  lock: function WeaveSync_lock(method ) {
    let savedMethod = method;
    let args = Array.prototype.slice.call(arguments, 1);

    return function WeaveLockWrapper() {
      let self = yield;
      let ret;

      this._dav.lock.async(this._dav, self.cb);
      let locked = yield;
      if (!locked)
        throw "Could not acquire lock";

      try {
        args.unshift(this, savedMethod, self.cb);
        Async.run.apply(Async, args);
        ret = yield;

      } catch (e) {
        throw e;

      } finally {
        this._dav.unlock.async(this._dav, self.cb);
        yield;
      }

      self.done(ret);
    };
  },

  
  
  localLock: function WeaveSync_localLock(method ) {
    let savedMethod = method;
    let args = Array.prototype.slice.call(arguments, 1);

    return function WeaveLocalLockWrapper() {
      let self = yield;
      let ret;

      if (this._dav.locked)
        throw "Could not acquire lock";
      this._dav.allowLock = false;

      try {
        args.unshift(this, savedMethod, self.cb);
        Async.run.apply(Async, args);
        ret = yield;
      }
      catch (e) { throw e; }
      finally { this._dav.allowLock = true; }

      self.done(ret);
    };
  }
}
