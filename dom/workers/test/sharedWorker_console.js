



"use strict";

onconnect = function(evt) {
  console.profile("Hello profiling from a SharedWorker!");
  console.log("Hello world from a SharedWorker!");
  evt.ports[0].postMessage('ok!');
}
