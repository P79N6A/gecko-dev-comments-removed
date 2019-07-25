



var counter = 0;

var interval = setInterval(function() {
  postMessage(++counter);
}, 100);

onmessage = function(event) {
  clearInterval(interval);
}
