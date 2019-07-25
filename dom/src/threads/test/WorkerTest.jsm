




































let EXPORTED_SYMBOLS = [
  "WorkerTest"
];

Components.utils.import("resource://gre/modules/XPCOMUtils.jsm");


XPCOMUtils.defineLazyServiceGetter(this, "workerFactory",
                                   "@mozilla.org/threads/workerfactory;1",
                                   "nsIWorkerFactory");

const WorkerTest = {
  go: function(message, messageCallback, errorCallback) {
    let worker = workerFactory.newChromeWorker("WorkerTest_worker.js");
    worker.onmessage = messageCallback;
    worker.onerror = errorCallback;
    worker.postMessage(message);
  }
};
