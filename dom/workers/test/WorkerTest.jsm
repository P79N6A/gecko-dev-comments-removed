



this.EXPORTED_SYMBOLS = [
  "WorkerTest"
];

this.WorkerTest = {
  go: function(message, messageCallback, errorCallback) {
    let worker = new ChromeWorker("WorkerTest_worker.js");
    worker.onmessage = messageCallback;
    worker.onerror = errorCallback;
    worker.postMessage(message);
    return worker;
  }
};
