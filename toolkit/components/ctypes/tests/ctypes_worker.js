





































importScripts("xpcshellTestHarnessAdaptor.js");

onmessage = function(event) {
  _WORKINGDIR_ = event.data;
if(false){
  importScripts("test_jsctypes.js");
  run_test();
}
  postMessage("Done!");
}
