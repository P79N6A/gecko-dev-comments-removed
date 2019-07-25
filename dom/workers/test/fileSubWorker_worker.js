



onmessage = function(event) {
  var worker = new Worker("file_worker.js");

  worker.postMessage(event.data);

  worker.onmessage = function(event) {
    postMessage(event.data);
  }

  worker.onerror = function(event) {
    postMessage(undefined);
  }
};
