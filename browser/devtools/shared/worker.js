


"use strict";

(function (factory) { 
  if (this.module && module.id.indexOf("worker") >= 0) { 
    const { Cc, Ci, ChromeWorker } = require("chrome");
    factory.call(this, require, exports, module, { Cc, Ci }, ChromeWorker);
  } else { 
      const { classes: Cc, interfaces: Ci, utils: Cu } = Components;
      const { devtools } = Cu.import("resource://gre/modules/devtools/Loader.jsm", {});
      this.isWorker = false;
      this.Promise = Cu.import("resource://gre/modules/Promise.jsm", {}).Promise;
      this.console = Cu.import("resource://gre/modules/devtools/Console.jsm", {}).console;
      factory.call(this, devtools.require, this, { exports: this }, { Cc, Ci }, ChromeWorker);
      this.EXPORTED_SYMBOLS = ["DevToolsWorker"];
  }
}).call(this, function (require, exports, module, { Ci, Cc }, ChromeWorker ) {

let MESSAGE_COUNTER = 0;











function DevToolsWorker (url) {
  this._worker = new ChromeWorker(url);
}
exports.DevToolsWorker = DevToolsWorker;












DevToolsWorker.prototype.performTask = function DevToolsWorkerPerformTask (task, data) {
  if (this._destroyed) {
    return Promise.reject("Cannot call performTask on a destroyed DevToolsWorker");
  }
  let worker = this._worker;
  let id = ++MESSAGE_COUNTER;
  worker.postMessage({ task, id, data });

  return new Promise(function (resolve, reject) {
    worker.addEventListener("message", function listener({ data }) {
      if (data.id !== id) {
        return;
      }
      worker.removeEventListener("message", listener);
      if (data.error) {
        reject(data.error);
      } else {
        resolve(data.response);
      }
    });
  });
}




DevToolsWorker.prototype.destroy = function DevToolsWorkerDestroy () {
  this._worker.terminate();
  this._worker = null;
  this._destroyed = true;
};




















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
    const { createTask } = require("resource:///modules/devtools/shared/worker-helper");
    createTask(self, "workerifiedTask", ${fn.toString()});
  `;
}

});
