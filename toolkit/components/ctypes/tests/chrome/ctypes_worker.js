




importScripts("xpcshellTestHarnessAdaptor.js");

onmessage = function(event) {
  _WORKINGDIR_ = event.data.dir;
  _OS_ = event.data.os;
  importScripts("test_jsctypes.js");
  run_test();
  postMessage("Done!");
}
