
















"use strict";

this.EXPORTED_SYMBOLS = ["BasePromiseWorker"];

const Cu = Components.utils;
const Ci = Components.interfaces;

Cu.import("resource://gre/modules/XPCOMUtils.jsm", this);

XPCOMUtils.defineLazyModuleGetter(this, "Promise",
  "resource://gre/modules/Promise.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "Task",
  "resource://gre/modules/Task.jsm");







function Queue() {
  this._array = [];
};
Queue.prototype = {
  pop: function pop() {
    return this._array.shift();
  },
  push: function push(x) {
    return this._array.push(x);
  },
  isEmpty: function isEmpty() {
    return this._array.length == 0;
  }
};





const EXCEPTION_CONSTRUCTORS = {
  EvalError: function(error) {
    let result = new EvalError(error.message, error.fileName, error.lineNumber);
    result.stack = error.stack;
    return result;
  },
  InternalError: function(error) {
    let result = new InternalError(error.message, error.fileName, error.lineNumber);
    result.stack = error.stack;
    return result;
  },
  RangeError: function(error) {
    let result = new RangeError(error.message, error.fileName, error.lineNumber);
    result.stack = error.stack;
    return result;
  },
  ReferenceError: function(error) {
    let result = new ReferenceError(error.message, error.fileName, error.lineNumber);
    result.stack = error.stack;
    return result;
  },
  SyntaxError: function(error) {
    let result = new SyntaxError(error.message, error.fileName, error.lineNumber);
    result.stack = error.stack;
    return result;
  },
  TypeError: function(error) {
    let result = new TypeError(error.message, error.fileName, error.lineNumber);
    result.stack = error.stack;
    return result;
  },
  URIError: function(error) {
    let result = new URIError(error.message, error.fileName, error.lineNumber);
    result.stack = error.stack;
    return result;
  },
  StopIteration: function() {
    return StopIteration;
  }
};

















this.BasePromiseWorker = function(url) {
  if (typeof url != "string") {
    throw new TypeError("Expecting a string");
  }
  this._url = url;

  










  this.ExceptionHandlers = Object.create(EXCEPTION_CONSTRUCTORS);

  










  this._queue = new Queue();

  




  this._id = 0;

  


  this.launchTimeStamp = null;

  


  this.workerTimeStamps = null;
};
this.BasePromiseWorker.prototype = {
  log: function() {
    
  },

  


  get _worker() {
    delete this._worker;
    let worker = new ChromeWorker(this._url);
    Object.defineProperty(this, "_worker", {value:
      worker
    });

    
    
    this.launchTimeStamp = Date.now();

    











    worker.onerror = error => {
      this.log("Received uncaught error from worker", error.message, error.filename, error.lineno);
      error.preventDefault();
      let {deferred} = this._queue.pop();
      deferred.reject(error);
    };

    















    worker.onmessage = msg => {
      this.log("Received message from worker", msg.data);
      let handler = this._queue.pop();
      let deferred = handler.deferred;
      let data = msg.data;
      if (data.id != handler.id) {
        throw new Error("Internal error: expecting msg " + handler.id + ", " +
                        " got " + data.id + ": " + JSON.stringify(msg.data));
      }
      if ("timeStamps" in data) {
        this.workerTimeStamps = data.timeStamps;
      }
      if ("ok" in data) {
        
        deferred.resolve(data);
      } else if ("fail" in data) {
        
        
        deferred.reject(new WorkerError(data.fail));
      }
    };
    return worker;
  },

  

















  post: function(fun, args, closure, transfers) {
    return Task.spawn(function* postMessage() {
      
      if (args) {
        args = yield Promise.resolve(Promise.all(args));
      }
      if (transfers) {
        transfers = yield Promise.resolve(Promise.all(transfers));
      }

      let id = ++this._id;
      let message = {fun: fun, args: args, id: id};
      this.log("Posting message", message);
      try {
        this._worker.postMessage(message, ...[transfers]);
      } catch (ex if typeof ex == "number") {
        this.log("Could not post message", message, "due to xpcom error", ex);
        
        throw new Components.Exception("Error in postMessage", ex);
      } catch (ex) {
        this.log("Could not post message", message, "due to error", ex);
        throw ex;
      }

      let deferred = Promise.defer();
      this._queue.push({deferred:deferred, closure: closure, id: id});
      this.log("Message posted");

      let reply;
      let isError = false;
      try {
        this.log("Expecting reply");
        reply = yield deferred.promise;
      } catch (error) {
        this.log("Got error", error);
        reply = error;
        isError = true;

        if (error instanceof WorkerError) {
          
          throw this.ExceptionHandlers[error.data.exn](error.data);
        }

        if (error instanceof ErrorEvent) {
          
          this.log("Error serialized by DOM", error.message, error.filename, error.lineno);
          throw new Error(error.message, error.filename, error.lineno);
        }

        
        throw error;
      }

      
      let options = null;
      if (args) {
        options = args[args.length - 1];
      }

      
      if (!options ||
          typeof options !== "object" ||
          !("outExecutionDuration" in options)) {
        return reply.ok;
      }
      
      
      
      if (!("durationMs" in reply)) {
        return reply.ok;
      }
      
      
      
      
      let durationMs = Math.max(0, reply.durationMs);
      
      if (typeof options.outExecutionDuration == "number") {
        options.outExecutionDuration += durationMs;
      } else {
        options.outExecutionDuration = durationMs;
      }
      return reply.ok;

    }.bind(this));
  }
};






function WorkerError(data) {
  this.data = data;
};
