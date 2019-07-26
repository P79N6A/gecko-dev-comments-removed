
























const Cc = Components.classes;
const Ci = Components.interfaces;
const Cu = Components.utils;
const Cr = Components.results;

Cu.import("resource://testing-common/httpd.js");



redirectHook = "http-on-modify-request";

var httpServer = null, httpServer2 = null;



var baitPath = "/bait";
var baitURI = "http://localhost:4444" + baitPath;
var baitText = "you got the worm";

var redirectedPath = "/switch";
var redirectedURI = "http://localhost:4444" + redirectedPath;
var redirectedText = "worms are not tasty";



var bait2Path = "/bait2";
var bait2URI = "http://localhost:4444" + bait2Path;
var redirected2URI = "http://localhost:4445" + redirectedPath;



var bait3Path = "/bait3";
var bait3URI = "http://localhost:4444" + bait3Path;



var bait4Path = "/bait4";
var bait4URI = "http://localhost:4444" + bait4Path;

var testHeaderName = "X-Redirected-By-Script"
var testHeaderVal = "Success";
var testHeaderVal2 = "Success on server 2";

function make_channel(url, callback, ctx) {
  var ios = Cc["@mozilla.org/network/io-service;1"].
            getService(Ci.nsIIOService);
  return ios.newChannel(url, "", null);
}

function baitHandler(metadata, response)
{
  
  response.setHeader("Content-Type", "text/html", false);
  response.bodyOutputStream.write(baitText, baitText.length);
}

function redirectedHandler(metadata, response)
{
  response.setHeader("Content-Type", "text/html", false);
  response.bodyOutputStream.write(redirectedText, redirectedText.length);
  response.setHeader(testHeaderName, testHeaderVal);
}

function redirected2Handler(metadata, response)
{
  response.setHeader("Content-Type", "text/html", false);
  response.bodyOutputStream.write(redirectedText, redirectedText.length);
  response.setHeader(testHeaderName, testHeaderVal2);
}

function bait3Handler(metadata, response)
{
  response.setHeader("Content-Type", "text/html", false);
  response.setStatusLine(metadata.httpVersion, 302, "Found");
  response.setHeader("Location", baitURI);
}

function Redirector()
{
  this.register();
}

Redirector.prototype = {
  
  
  register: function()
  {
    Cc["@mozilla.org/observer-service;1"].
      getService(Ci.nsIObserverService).
      addObserver(this, redirectHook, true);
  },

  QueryInterface: function(iid)
  {
    if (iid.equals(Ci.nsIObserver) ||
        iid.equals(Ci.nsISupportsWeakReference) ||
        iid.equals(Ci.nsISupports))
      return this;
    throw Components.results.NS_NOINTERFACE;
  },

  observe: function(subject, topic, data)
  {
    if (topic == redirectHook) {
      if (!(subject instanceof Ci.nsIHttpChannel))
        do_throw(redirectHook + " observed a non-HTTP channel");
      var channel = subject.QueryInterface(Ci.nsIHttpChannel);
      var ioservice = Cc["@mozilla.org/network/io-service;1"].
                        getService(Ci.nsIIOService);
      var target = null;
      if (channel.URI.spec == baitURI)  target = redirectedURI;
      if (channel.URI.spec == bait2URI) target = redirected2URI;
      if (channel.URI.spec == bait4URI) target = baitURI;
Â      
      if (target) {
        var tURI = ioservice.newURI(target, null, null);
        try {
          channel.redirectTo(tURI);
        } catch (e) {
          do_throw("Exception in redirectTo " + e + "\n");
        }
      }
    }
  }
}

function makeAsyncTest(uri, headerValue, nextTask)
{
  

  
  
  var verifier = function(req, buffer)
  {
    if (!(req instanceof Ci.nsIHttpChannel))
      do_throw(req + " is not an nsIHttpChannel, catastrophe imminent!");

    var httpChannel = req.QueryInterface(Ci.nsIHttpChannel);
    do_check_eq(httpChannel.getResponseHeader(testHeaderName), headerValue);
    do_check_eq(buffer, redirectedText);
    nextTask();
  };

  
  var test = function()
  {
    var chan = make_channel(uri);
    chan.asyncOpen(new ChannelListener(verifier), null);
  };
  return test;
}






var testViaAsyncOpen4 = makeAsyncTest(bait4URI, testHeaderVal, done);
var testViaAsyncOpen3 = makeAsyncTest(bait3URI, testHeaderVal, testViaAsyncOpen4);
var testViaAsyncOpen2 = makeAsyncTest(bait2URI, testHeaderVal2, testViaAsyncOpen3);
var testViaAsyncOpen  = makeAsyncTest(baitURI,  testHeaderVal, testViaAsyncOpen2);

function testViaXHR()
{
  runXHRTest(baitURI,  testHeaderVal);
  runXHRTest(bait2URI, testHeaderVal2);
  runXHRTest(bait3URI, testHeaderVal);
  runXHRTest(bait4URI, testHeaderVal);
}

function runXHRTest(uri, headerValue)
{
  
  
  var xhr = Cc["@mozilla.org/xmlextras/xmlhttprequest;1"];

  var req = xhr.createInstance(Ci.nsIXMLHttpRequest);
  req.open("GET", uri, false);
  req.send();
  do_check_eq(req.getResponseHeader(testHeaderName), headerValue);
  do_check_eq(req.response, redirectedText);
}

function done()
{
  httpServer.stop(
    function ()
    {
      httpServer2.stop(do_test_finished);
    }
  );
}

var redirector = new Redirector();

function run_test()
{
  httpServer = new HttpServer();
  httpServer.registerPathHandler(baitPath, baitHandler);
  httpServer.registerPathHandler(bait2Path, baitHandler);
  httpServer.registerPathHandler(bait3Path, bait3Handler);
  httpServer.registerPathHandler(bait4Path, baitHandler);
  httpServer.registerPathHandler(redirectedPath, redirectedHandler);
  httpServer.start(4444);
  httpServer2 = new HttpServer();
  httpServer2.registerPathHandler(redirectedPath, redirected2Handler);
  httpServer2.start(4445);

  testViaXHR();
  testViaAsyncOpen();  

  do_test_pending();
}
