






"use strict";

this.EXPORTED_SYMBOLS = ["PromiseWorker"];


Components.utils.import("resource://gre/modules/commonjs/promise/core.js", this);







let Queue = function Queue() {
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











function PromiseWorker(url, log) {
  if (typeof url != "string") {
    throw new TypeError("Expecting a string");
  }
  if (log != null && typeof log != "function") {
    throw new TypeError("Expecting either null or a function");
  }
  this._log = log;
  this._url = url;

  










  this._queue = new Queue();

  




  this._id = 0;
}
PromiseWorker.prototype = {
  


  get _worker() {
    delete this._worker;
    let worker = new ChromeWorker(this._url);
    let self = this;
    Object.defineProperty(this, "_worker", {value:
      worker
    });

    











    worker.onerror = function onerror(error) {
      if (self._log) {
        self._log("Received uncaught error from worker", JSON.stringify(error.message), error.message);
      }
      error.preventDefault();
      let {deferred} = self._queue.pop();
      deferred.reject(error);
    };

    












    worker.onmessage = function onmessage(msg) {
      if (self._log) {
        self._log("Received message from worker", JSON.stringify(msg.data));
      }
      let handler = self._queue.pop();
      let deferred = handler.deferred;
      let data = msg.data;
      if (data.id != handler.id) {
        throw new Error("Internal error: expecting msg " + handler.id + ", " +
                        " got " + data.id + ": " + JSON.stringify(msg.data));
      }
      if ("ok" in data) {
        deferred.resolve(data.ok);
      } else if ("StopIteration" in data) {
        
        deferred.reject(StopIteration);
      } if ("fail" in data) {
        
        
        deferred.reject(new PromiseWorker.WorkerError(data.fail));
      }
    };
    return worker;
  },

  









  post: function post(fun, array, closure) {
    let deferred = Promise.defer();
    let id = ++this._id;
    let message = {fun: fun, args: array, id: id};
    if (this._log) {
      this._log("Posting message", JSON.stringify(message));
    }
    this._queue.push({deferred:deferred, closure: closure, id: id});
    this._worker.postMessage(message);
    if (this._log) {
      this._log("Message posted");
    }
    return deferred.promise;
  }
};






PromiseWorker.WorkerError = function WorkerError(data) {
  this.data = data;
};

this.PromiseWorker = PromiseWorker;
