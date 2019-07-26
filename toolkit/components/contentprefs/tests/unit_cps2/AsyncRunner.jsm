



let EXPORTED_SYMBOLS = [
  "AsyncRunner",
];

const { interfaces: Ci, classes: Cc } = Components;

function AsyncRunner(callbacks) {
  this._callbacks = callbacks;
  this._iteratorQueue = [];

  
  Cc["@mozilla.org/consoleservice;1"].
    getService(Ci.nsIConsoleService).
    registerListener(this);
}

AsyncRunner.prototype = {

  appendIterator: function AR_appendIterator(iter) {
    this._iteratorQueue.push(iter);
  },

  next: function AR_next() {
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

  destroy: function AR_destroy() {
    Cc["@mozilla.org/consoleservice;1"].
      getService(Ci.nsIConsoleService).
      unregisterListener(this);
    this.destroy = function AR_alreadyDestroyed() {};
  },

  observe: function AR_consoleServiceListener(msg) {
    if (msg instanceof Ci.nsIScriptError)
      this._callbacks.consoleError(msg);
  },
};
