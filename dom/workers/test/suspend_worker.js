



var counter = 0;

var interval = setInterval(function() {
  dump("WorkerAlive\n"); 
  postMessage(++counter);
}, 100);

onmessage = function(event) {
  clearInterval(interval);
}
