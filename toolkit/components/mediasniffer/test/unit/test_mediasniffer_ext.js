



const Ci = Components.interfaces;
const Cu = Components.utils;
const Cc = Components.classes;
const CC = Components.Constructor;

var BinaryOutputStream = CC("@mozilla.org/binaryoutputstream;1",
                            "nsIBinaryOutputStream",
                            "setOutputStream");

Cu.import("resource://testing-common/httpd.js");

var httpserver = new HttpServer();

var testRan = 0;


const tests = [
  
  { path: "data/file.webm", expected: "video/webm" },
  { path: "data/file.mkv", expected: "application/octet-stream" },
  
  
  
  { path: "data/id3tags.mp3", expected: "audio/mpeg" },
  { path: "data/notags.mp3", expected: "audio/mpeg" },
  
  { path: "data/detodos.mp3", expected: "audio/mpeg" },
  
  { path: "data/notags-bad.mp3", expected: "application/octet-stream" },
  
  { path: "data/notags-scan.mp3", expected: "application/octet-stream" },
  
  { path: "data/he_free.mp3", expected: "application/octet-stream" },
  
  { path: "data/fl10.mp2", expected: "application/octet-stream" },
  
  { path: "data/ff-inst.exe", expected: "application/octet-stream" },
  
  { path: "data/bug1079747.mp4", expected: "application/octet-stream" },
];


var listener = {
  onStartRequest: function(request, context) {
    do_print("Sniffing " + tests[testRan].path);
    do_check_eq(request.QueryInterface(Ci.nsIChannel).contentType, tests[testRan].expected);
  },

  onDataAvailable: function(request, context, stream, offset, count) {
    try {
      var bis = Components.classes["@mozilla.org/binaryinputstream;1"]
                          .createInstance(Components.interfaces.nsIBinaryInputStream);
      bis.setInputStream(stream);
      var array = bis.readByteArray(bis.available());
    } catch (ex) {
      do_throw("Error in onDataAvailable: " + ex);
    }
  },

  onStopRequest: function(request, context, status) {
    testRan++;
    runNext();
  }
};

function setupChannel(url) {
  var ios = Components.classes["@mozilla.org/network/io-service;1"].
                       getService(Ci.nsIIOService);
  var chan = ios.newChannel("http://localhost:" +
                           httpserver.identity.primaryPort + url, "", null);
  var httpChan = chan.QueryInterface(Components.interfaces.nsIHttpChannel);
  return httpChan;
}

function runNext() {
  if (testRan == tests.length) {
    do_test_finished();
    return;
  }
  var channel = setupChannel("/");
  channel.asyncOpen(listener, channel, null);
}

function getFileContents(aFile) {
  const PR_RDONLY = 0x01;
  var fileStream = Cc["@mozilla.org/network/file-input-stream;1"]
                      .createInstance(Ci.nsIFileInputStream);
  fileStream.init(aFile, 1, -1, null);
  var bis = Components.classes["@mozilla.org/binaryinputstream;1"]
                      .createInstance(Components.interfaces.nsIBinaryInputStream);
  bis.setInputStream(fileStream);

  var data = bis.readByteArray(bis.available());

  return data;
}

function handler(metadata, response) {
  response.setStatusLine(metadata.httpVersion, 200, "OK");
  
  response.setHeader("Content-Type", "", false);
  var body = getFileContents(do_get_file(tests[testRan].path));
  var bos = new BinaryOutputStream(response.bodyOutputStream);
  bos.writeByteArray(body, body.length);
}

function run_test() {
  
  httpserver.registerPathHandler("/", handler);
  httpserver.start(-1);
  do_test_pending();
  try {
    runNext();
  } catch (e) {
    print("ERROR - " + e + "\n");
  }
}
