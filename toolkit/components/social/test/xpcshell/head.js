



const { classes: Cc, interfaces: Ci, utils: Cu, results: Cr } = Components;
Cu.import("resource://gre/modules/Services.jsm");

const MANIFEST_PREFS = Services.prefs.getBranch("social.manifest.");

function AsyncRunner() {
  do_test_pending();
  do_register_cleanup((function () this.destroy()).bind(this));

  this._callbacks = {
    done: do_test_finished,
    error: function (err) {
      
      
      if (err !== Cr.NS_ERROR_ABORT) {
        if (err.stack) {
          err = err + " - See following stack:\n" + err.stack +
                      "\nUseless do_throw stack";
        }
        do_throw(err);
      }
    },
    consoleError: function (scriptErr) {
      
      let filename = scriptErr.sourceName || scriptErr.toString() || "";
      if (filename.indexOf("/toolkit/components/social/") >= 0)
        do_throw(scriptErr);
    },
  };
  this._iteratorQueue = [];

  
  
  Cc["@mozilla.org/consoleservice;1"].
    getService(Ci.nsIConsoleService).
    registerListener(this);
}

AsyncRunner.prototype = {

  appendIterator: function appendIterator(iter) {
    this._iteratorQueue.push(iter);
  },

  next: function next() {
    if (!this._iteratorQueue.length) {
      this.destroy();
      this._callbacks.done();
      return;
    }

    
    
    let args = [arguments.length <= 1 ? arguments[0] : Array.slice(arguments)];
    try {
      var val = this._iteratorQueue[0].send.apply(this._iteratorQueue[0], args);
    }
    catch (err if err instanceof StopIteration) {
      this._iteratorQueue.shift();
      this.next();
      return;
    }
    catch (err) {
      this._callbacks.error(err);
    }

    
    
    if (val) {
      if (typeof(val) != "boolean")
        this._iteratorQueue.unshift(val);
      this.next();
    }
  },

  destroy: function destroy() {
    Cc["@mozilla.org/consoleservice;1"].
      getService(Ci.nsIConsoleService).
      unregisterListener(this);
    this.destroy = function alreadyDestroyed() {};
  },

  observe: function observe(msg) {
    if (msg instanceof Ci.nsIScriptError)
      this._callbacks.consoleError(msg);
  },
};
