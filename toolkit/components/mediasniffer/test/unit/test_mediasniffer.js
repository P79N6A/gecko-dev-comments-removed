



const Ci = Components.interfaces;
const Cu = Components.utils;

Cu.import("resource://testing-common/httpd.js");

const PATH = "/file.meh";
var httpserver = new HttpServer();


const data = "OggS\0meeeh.";
var testRan = 0;




const tests = [
  
  
  { contentType: "",
    expectedContentType: "application/ogg",
    flags: Ci.nsIChannel.LOAD_TREAT_APPLICATION_OCTET_STREAM_AS_UNKNOWN },
  { contentType: "application/octet-stream",
    expectedContentType: "application/ogg",
    flags: Ci.nsIChannel.LOAD_TREAT_APPLICATION_OCTET_STREAM_AS_UNKNOWN },
  { contentType: "application/something",
    expectedContentType: "application/something",
    flags: Ci.nsIChannel.LOAD_TREAT_APPLICATION_OCTET_STREAM_AS_UNKNOWN },
  
  
  { contentType: "application/octet-stream",
    expectedContentType: "application/ogg",
    flags: Ci.nsIChannel.LOAD_CALL_CONTENT_SNIFFERS },
  { contentType: "",
    expectedContentType: "application/ogg",
    flags: Ci.nsIChannel.LOAD_CALL_CONTENT_SNIFFERS },
];


var listener = {
  onStartRequest: function(request, context) {
    do_check_eq(request.QueryInterface(Ci.nsIChannel).contentType,
                tests[testRan].expectedContentType);
  },

  onDataAvailable: function(request, context, stream, offset, count) {
    try {
      var bis = Components.classes["@mozilla.org/binaryinputstream;1"]
                          .createInstance(Components.interfaces.nsIBinaryInputStream);
      bis.setInputStream(stream);
      bis.readByteArray(bis.available());
    } catch (ex) {
      do_throw("Error in onDataAvailable: " + ex);
    }
  },

  onStopRequest: function(request, context, status) {
    testRan++;
    runNext();
  }
};

function setupChannel(url, flags)
{
  var ios = Components.classes["@mozilla.org/network/io-service;1"].
                       getService(Ci.nsIIOService);
  var chan = ios.newChannel("http://localhost:" +
                           httpserver.identity.primaryPort + url, "", null);
  chan.loadFlags |= flags;
  var httpChan = chan.QueryInterface(Components.interfaces.nsIHttpChannel);
  return httpChan;
}

function runNext() {
  if (testRan == tests.length) {
    do_test_finished();
    return;
  }
  var channel = setupChannel(PATH, tests[testRan].flags);
  httpserver.registerPathHandler(PATH, function(request, response) {
    response.setHeader("Content-Type", tests[testRan].contentType, false);
    response.bodyOutputStream.write(data, data.length);
  });
  channel.asyncOpen(listener, channel, null);
}

function run_test() {
  httpserver.start(-1);
  do_test_pending();
  try {
    runNext();
  } catch (e) {
    print("ERROR - " + e + "\n");
  }
}
