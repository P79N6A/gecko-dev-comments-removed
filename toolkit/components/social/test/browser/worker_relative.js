
onconnect = function(e) {
  let port = e.ports[0];
  let req;
  try {
    importScripts("relative_import.js");
    
    if (testVar == "oh hai" && testFunc() == "oh hai") {
      port.postMessage({topic: "done", result: "ok"});
    } else {
      port.postMessage({topic: "done", result: "import worked but global is not available"});
    }
    return;
  } catch(e) {
    port.postMessage({topic: "done", result: "FAILED to importScripts, " + e.toString() });
    return;
  }
  port.postMessage({topic: "done", result: "FAILED to importScripts, no exception" });
}
