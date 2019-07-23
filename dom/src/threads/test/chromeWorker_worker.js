




































if (!ctypes) {
  throw "No ctypes!";
}

let worker = new ChromeWorker("chromeWorker_subworker.js");
worker.onmessage = function(event) {
  postMessage(event.data);
}
worker.postMessage("Go");
