





































importScripts("xpcshellTestHarnessAdaptor.js");

onmessage = function(event) {
  _WORKINGDIR_ = event.data[0];
  importScripts("file://" + event.data[0] + "/" + event.data[1]);
  run_test();
  postMessage("Done!");
}
