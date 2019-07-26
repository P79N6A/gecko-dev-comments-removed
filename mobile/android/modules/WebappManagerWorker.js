



importScripts("resource://gre/modules/osfile.jsm");

function log(message) {
  dump("WebManagerWorker " + message + "\n");
}

onmessage = function(event) {
  let { url, path } = event.data;

  let file = OS.File.open(path, { truncate: true });
  let request = new XMLHttpRequest({ mozSystem: true });

  request.open("GET", url, true);
  request.responseType = "moz-chunked-arraybuffer";

  request.onprogress = function(event) {
    log("onprogress: received " + request.response.byteLength + " bytes");
    let bytesWritten = file.write(new Uint8Array(request.response));
    log("onprogress: wrote " + bytesWritten + " bytes");
  };

  request.onreadystatechange = function(event) {
    log("onreadystatechange: " + request.readyState);

    if (request.readyState == 4) {
      file.close();

      if (request.status == 200 || request.status == 0) {
        postMessage({ type: "success" });
      } else {
        postMessage({ type: "failure", message: request.statusText });
      }
    }
  };

  request.send(null);
}
