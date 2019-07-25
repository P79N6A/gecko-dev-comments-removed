



































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

        args = savedArgs.concat(args);
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
    let savedArgs = Array.prototype.slice.call(arguments, 1);

    return function WeaveLockWrapper( ) {
      let self = yield;
      let ret;
      let args = Array.prototype.slice.call(arguments);

      DAV.lock.async(DAV, self.cb);
      let locked = yield;
      if (!locked)
        throw "Could not acquire lock";

      try {
        args = savedArgs.concat(args);
        args.unshift(this, savedMethod, self.cb);
        Async.run.apply(Async, args);
        ret = yield;

      } catch (e) {
        throw e;

      } finally {
        DAV.unlock.async(DAV, self.cb);
        yield;
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

      try {
        args = savedArgs.concat(args);
        args.unshift(this, savedMethod, self.cb);
        Async.run.apply(Async, args);
        ret = yield;
      }
      catch (e) { throw e; }
      finally { DAV.allowLock = true; }

      self.done(ret);
    };
  }
}
