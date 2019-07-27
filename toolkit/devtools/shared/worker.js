


"use strict";

(function (factory) { 
  if (this.module && module.id.indexOf("worker") >= 0) { 
    const { Cc, Ci, Cu, ChromeWorker } = require("chrome");
    const dumpn = require("devtools/toolkit/DevToolsUtils").dumpn;
    factory.call(this, require, exports, module, { Cc, Ci, Cu }, ChromeWorker, dumpn);
  } else { 
    const { classes: Cc, interfaces: Ci, utils: Cu } = Components;
    const { devtools } = Cu.import("resource://gre/modules/devtools/Loader.jsm", {});
    this.isWorker = false;
    this.Promise = Cu.import("resource://gre/modules/Promise.jsm", {}).Promise;
    this.console = Cu.import("resource://gre/modules/devtools/Console.jsm", {}).console;
    factory.call(
      this, devtools.require, this, { exports: this },
      { Cc, Ci, Cu }, ChromeWorker, null
    );
    this.EXPORTED_SYMBOLS = ["DevToolsWorker"];
  }
}).call(this, function (require, exports, module, { Ci, Cc }, ChromeWorker, dumpn) {

let MESSAGE_COUNTER = 0;















function DevToolsWorker (url, opts) {
  opts = opts || {};
  this._worker = new ChromeWorker(url);
  this._verbose = opts.verbose;
  this._name = opts.name;

  this._worker.addEventListener("error", this.onError, false);
}
exports.DevToolsWorker = DevToolsWorker;












DevToolsWorker.prototype.performTask = function (task, data) {
  if (this._destroyed) {
    return Promise.reject("Cannot call performTask on a destroyed DevToolsWorker");
  }
  let worker = this._worker;
  let id = ++MESSAGE_COUNTER;
  let payload = { task, id, data };

  if(this._verbose && dumpn) {
    dumpn("Sending message to worker" +
          (this._name ? (" (" + this._name + ")") : "" ) +
          ": " +
          JSON.stringify(payload, null, 2));
  }
  worker.postMessage(payload);

  return new Promise((resolve, reject) => {
    let listener = ({ data }) => {
      if(this._verbose && dumpn) {
        dumpn("Received message from worker" +
              (this._name ? (" (" + this._name + ")") : "" ) +
              ": " +
              JSON.stringify(data, null, 2));
      }

      if (data.id !== id) {
        return;
      }
      worker.removeEventListener("message", listener);
      if (data.error) {
        reject(data.error);
      } else {
        resolve(data.response);
      }
    };

    worker.addEventListener("message", listener);
  });
}




DevToolsWorker.prototype.destroy = function () {
  this._worker.terminate();
  this._worker = null;
  this._destroyed = true;
};

DevToolsWorker.prototype.onError = function({ message, filename, lineno }) {
  Cu.reportError(new Error(message + " @ " + filename + ":" + lineno));
}




















function workerify (fn) {
  console.warn(`\`workerify\` should only be used in tests or measuring performance.
  This creates an object URL on the browser window, and should not be used in production.`)
  
  
  let { getMostRecentBrowserWindow } = require("sdk/window/utils");
  let { URL, Blob } = getMostRecentBrowserWindow();
  let stringifiedFn = createWorkerString(fn);
  let blob = new Blob([stringifiedFn]);
  let url = URL.createObjectURL(blob);
  let worker = new DevToolsWorker(url);

  let wrapperFn = data => worker.performTask("workerifiedTask", data);

  wrapperFn.destroy = function () {
    URL.revokeObjectURL(url);
    worker.destroy();
  };

  return wrapperFn;
}
exports.workerify = workerify;





function createWorkerString (fn) {
  return `importScripts("resource://gre/modules/workers/require.js");
    const { createTask } = require("resource://gre/modules/devtools/shared/worker-helper");
    createTask(self, "workerifiedTask", ${fn.toString()});
  `;
}

});
