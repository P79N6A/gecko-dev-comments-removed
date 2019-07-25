




































onmessage = function(event) {
  let worker = new ChromeWorker("WorkerTest_subworker.js");
  worker.onmessage = function(event) {
    postMessage(event.data);
  }
  worker.postMessage(event.data);
}
