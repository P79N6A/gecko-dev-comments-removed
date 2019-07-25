const Cc = Components.classes;
const Ci = Components.interfaces;
const Cu = Components.utils;
const Cr = Components.results;

Cu.import("resource://testing-common/httpd.js");








var httpServer = null;

var fullLoopPath = "/fullLoop"; 
var fullLoopURI = "http://localhost:4444" + fullLoopPath;

var relativeLoopPath = "/relativeLoop"; 
var relativeLoopURI = "http://localhost:4444" + relativeLoopPath;


var emptyLoopPath = "/empty/";
var emptyLoopURI = "http://localhost:4444" + emptyLoopPath;

function make_channel(url, callback, ctx) {
  var ios = Cc["@mozilla.org/network/io-service;1"].
            getService(Ci.nsIIOService);
  return ios.newChannel(url, "", null);
}

function fullLoopHandler(metadata, response)
{
  response.setStatusLine(metadata.httpVersion, 301, "Moved");
  response.setHeader("Location", "http://localhost:4444/fullLoop", false);
}

function relativeLoopHandler(metadata, response)
{
  response.setStatusLine(metadata.httpVersion, 301, "Moved");
  response.setHeader("Location", "relativeLoop", false);
}

function emptyLoopHandler(metadata, response)
{
  
  
  response.seizePower();
  response.write("HTTP/1.0 301 Moved\r\n");
  response.write("Location: \r\n");
  response.write("Content-Length: 4\r\n");
  response.write("\r\n");
  response.write("oops");
  response.finish();
}

function testFullLoop(request, buffer)
{
  do_check_eq(request.status, Components.results.NS_ERROR_REDIRECT_LOOP);

  var chan = make_channel(relativeLoopURI);
  chan.asyncOpen(new ChannelListener(testRelativeLoop, null, CL_EXPECT_FAILURE),
                 null);
}

function testRelativeLoop(request, buffer)
{
  do_check_eq(request.status, Components.results.NS_ERROR_REDIRECT_LOOP);

  var chan = make_channel(emptyLoopURI);
  chan.asyncOpen(new ChannelListener(testEmptyLoop, null, CL_EXPECT_FAILURE),
                 null);
}

function testEmptyLoop(request, buffer)
{
  do_check_eq(request.status, Components.results.NS_ERROR_REDIRECT_LOOP);

  httpServer.stop(do_test_finished);
}

function run_test()
{
  httpServer = new HttpServer();
  httpServer.registerPathHandler(fullLoopPath, fullLoopHandler);
  httpServer.registerPathHandler(relativeLoopPath, relativeLoopHandler);
  httpServer.registerPathHandler(emptyLoopPath, emptyLoopHandler);
  httpServer.start(4444);

  var chan = make_channel(fullLoopURI);
  chan.asyncOpen(new ChannelListener(testFullLoop, null, CL_EXPECT_FAILURE),
                 null);
  do_test_pending();
}
