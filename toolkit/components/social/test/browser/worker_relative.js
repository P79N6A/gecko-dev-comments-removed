
onconnect = function(e) {
  let port = e.ports[0];
  let req;
  try {
    importScripts("relative_import.js");
    
    if (testVar != "oh hai" || testFunc() != "oh hai") {
      port.postMessage({topic: "done", result: "import worked but global is not available"});
      return;
    }

    
    
    try {
      causeError();
    } catch(e) {
      let fileName = e.fileName;
      if (fileName.startsWith("http") &&
          fileName.endsWith("/relative_import.js") &&
          e.lineNumber == 4)
        port.postMessage({topic: "done", result: "ok"});
      else
        port.postMessage({topic: "done", result: "invalid error location: " + fileName + ":" + e.lineNumber});
      return;
    }
  } catch(e) {
    port.postMessage({topic: "done", result: "FAILED to importScripts, " + e.toString() });
    return;
  }
  port.postMessage({topic: "done", result: "FAILED to importScripts, no exception" });
}
