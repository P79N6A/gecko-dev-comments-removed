const Cc = Components.classes;
const Ci = Components.interfaces;
const Cu = Components.utils;
const Cr = Components.results;

Cu.import("resource://testing-common/httpd.js");








var httpserver = null;

var randomPath1 = "/redirect-no-store/" + Math.random();
var randomURI1 = "http://localhost:4444" + randomPath1;
var randomPath2 = "/redirect-expires-past/" + Math.random();
var randomURI2 = "http://localhost:4444" + randomPath2;

function make_channel(url, callback, ctx) {
  var ios = Cc["@mozilla.org/network/io-service;1"].
            getService(Ci.nsIIOService);
  return ios.newChannel(url, "", null);
}

const responseBody = "response body";

function redirectHandler_NoStore(metadata, response)
{
  response.setStatusLine(metadata.httpVersion, 302, "Found");
  response.setHeader("Location", "http://localhost:4444/content", false);
  response.setHeader("Cache-control", "no-store");
  return;
}

function redirectHandler_ExpiresInPast(metadata, response)
{
  response.setStatusLine(metadata.httpVersion, 302, "Found");
  response.setHeader("Location", "http://localhost:4444/content", false);
  response.setHeader("Expires", "-1");
  return;
}

function contentHandler(metadata, response)
{
  response.setHeader("Content-Type", "text/plain");
  response.bodyOutputStream.write(responseBody, responseBody.length);
}

function check_response(path, request, buffer, expectedExpiration, continuation)
{
  do_check_eq(buffer, responseBody);
  
  asyncCheckCacheEntryPresence(path, "HTTP", Ci.nsICache.STORE_ON_DISK, false, expectedExpiration, function() {
  asyncCheckCacheEntryPresence(path, "HTTP", Ci.nsICache.STORE_IN_MEMORY, !expectedExpiration, expectedExpiration, continuation);
  });
}

function run_test_no_store()
{
  var chan = make_channel(randomURI1);
  chan.asyncOpen(new ChannelListener(function(request, buffer) {
    
    check_response(randomURI1, request, buffer, false, run_test_expires_past);
  }, null), null);
}

function run_test_expires_past()
{
  var chan = make_channel(randomURI2);
  chan.asyncOpen(new ChannelListener(function(request, buffer) {
    
    check_response(randomURI2, request, buffer, true, finish_test);
  }, null), null);
}

function finish_test()
{
  httpserver.stop(do_test_finished);
}

function run_test()
{
  do_get_profile();

  httpserver = new HttpServer();
  httpserver.registerPathHandler(randomPath1, redirectHandler_NoStore);
  httpserver.registerPathHandler(randomPath2, redirectHandler_ExpiresInPast);
  httpserver.registerPathHandler("/content", contentHandler);
  httpserver.start(4444);

  run_test_no_store();
  do_test_pending();
}
