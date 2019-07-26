Cu.import("resource://testing-common/httpd.js");








var httpserver = null;

var randomPath1 = "/redirect-no-store/" + Math.random();

XPCOMUtils.defineLazyGetter(this, "randomURI1", function() {
  return "http://localhost:" + httpserver.identity.primaryPort + randomPath1;
});

var randomPath2 = "/redirect-expires-past/" + Math.random();

XPCOMUtils.defineLazyGetter(this, "randomURI2", function() {
  return "http://localhost:" + httpserver.identity.primaryPort + randomPath2;
});

function make_channel(url, callback, ctx) {
  var ios = Cc["@mozilla.org/network/io-service;1"].
            getService(Ci.nsIIOService);
  return ios.newChannel(url, "", null);
}

const responseBody = "response body";

var redirectHandler_NoStore_calls = 0;
function redirectHandler_NoStore(metadata, response)
{
  response.setStatusLine(metadata.httpVersion, 302, "Found");
  response.setHeader("Location", "http://localhost:" +
                     httpserver.identity.primaryPort + "/content", false);
  response.setHeader("Cache-control", "no-store");
  ++redirectHandler_NoStore_calls;
  return;
}

var redirectHandler_ExpiresInPast_calls = 0;
function redirectHandler_ExpiresInPast(metadata, response)
{
  response.setStatusLine(metadata.httpVersion, 302, "Found");
  response.setHeader("Location", "http://localhost:" +
                     httpserver.identity.primaryPort + "/content", false);
  response.setHeader("Expires", "-1");
  ++redirectHandler_ExpiresInPast_calls;
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

  
  
  asyncOpenCacheEntry(path, "disk", Ci.nsICacheStorage.OPEN_READONLY, null, function(status, entry) {
    do_check_eq(status, 0);

    
    do_check_eq(entry.persistToDisk, expectedExpiration);

    
    var chan = make_channel(path);
    chan.asyncOpen(new ChannelListener(function(request, buffer) {
      do_check_eq(buffer, responseBody);

      if (expectedExpiration) {
        
        do_check_eq(redirectHandler_ExpiresInPast_calls, 2);
      }
      else {
        
        
        do_check_eq(redirectHandler_NoStore_calls, 2);
        do_check_true(!entry.persistToDisk);
      }

      continuation();
    }, null), null);
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
  httpserver.start(-1);

  run_test_no_store();
  do_test_pending();
}
