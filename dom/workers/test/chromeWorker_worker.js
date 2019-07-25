



if (!("ctypes" in self)) {
  throw "No ctypes!";
}


if (ctypes.toString() != "[object ctypes]") {
  throw "Bad ctypes object: " + ctypes.toString();
}

onmessage = function(event) {
  let worker = new ChromeWorker("chromeWorker_subworker.js");
  worker.onmessage = function(event) {
    postMessage(event.data);
  }
  worker.postMessage(event.data);
}
