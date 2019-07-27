












Cu.import("resource://testing-common/httpd.js");
Cu.import("resource://gre/modules/Services.jsm");

XPCOMUtils.defineLazyGetter(this, "URL", function() {
  return "http://localhost:" + httpServer.identity.primaryPort;
});

var httpServer = null;

function make_channel(url, callback, ctx) {
  var ios = Cc["@mozilla.org/network/io-service;1"].
            getService(Ci.nsIIOService);
  return ios.newChannel(url, "", null);
}

const responseBody = "response body";

function contentHandler(metadata, response)
{
  response.setHeader("Content-Type", "text/plain");
  response.setHeader("ETag", "Just testing");
  response.setHeader("Cache-Control", "max-age=99999");
  response.setHeader("Accept-Ranges", "bytes");
  response.setHeader("Content-Length", "" + responseBody.length);
  if (metadata.hasHeader("If-Range")) {
	  response.setStatusLine(metadata.httpVersion, 206, "Partial Content");
	  response.setHeader("Content-Range", "0-12/13");
  }
  response.bodyOutputStream.write(responseBody, responseBody.length);
}

function run_test()
{
  do_get_profile();

  Services.prefs.setIntPref("browser.cache.disk.max_entry_size", 0);

  httpServer = new HttpServer();
  httpServer.registerPathHandler("/content", contentHandler);
  httpServer.start(-1);

  var chan1 = make_channel(URL + "/content");
  chan1.asyncOpen(new ChannelListener(firstTimeThrough, null), null);
  var chan2 = make_channel(URL + "/content");
  chan2.asyncOpen(new ChannelListener(secondTimeThrough, null), null);

  do_test_pending();
}

function firstTimeThrough(request, buffer)
{
  do_check_eq(buffer, responseBody);
}

function secondTimeThrough(request, buffer)
{
  do_check_eq(buffer, responseBody);
  httpServer.stop(do_test_finished);
}
