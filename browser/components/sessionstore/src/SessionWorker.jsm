



"use strict";





const Cu = Components.utils;
const Cc = Components.classes;
const Ci = Components.interfaces;
const Cr = Components.results;

Cu.import("resource://gre/modules/osfile/_PromiseWorker.jsm", this);
Cu.import("resource://gre/modules/osfile.jsm", this);

this.EXPORTED_SYMBOLS = ["SessionWorker"];

this.SessionWorker = (function () {
  let worker = new PromiseWorker("resource:///modules/sessionstore/SessionWorker.js",
    OS.Shared.LOG.bind("SessionWorker"));
  return {
    post: function post(...args) {
      let promise = worker.post.apply(worker, args);
      return promise.then(
        null,
        function onError(error) {
          
          if (error instanceof PromiseWorker.WorkerError) {
            throw OS.File.Error.fromMsg(error.data);
          }
          
          if (error instanceof ErrorEvent) {
            throw new Error(error.message, error.filename, error.lineno);
          }
          throw error;
        }
      );
    }
  };
})();
