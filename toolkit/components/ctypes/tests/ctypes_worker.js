





































importScripts("xpcshellTestHarnessAdaptor.js");

onmessage = function(event) {
  _WORKINGDIR_ = event.data;
  importScripts("test_jsctypes.js");
  run_test();
  postMessage("Done!");
}
