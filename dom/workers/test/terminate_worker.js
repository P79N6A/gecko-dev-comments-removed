



onclose = function() {
  postMessage("Closed!");
}

onmessage = function(event) {
  throw "No messages should reach me!";
}

setInterval(function() { dump("WorkerAlive\n"); postMessage("Still alive!"); }, 100);
