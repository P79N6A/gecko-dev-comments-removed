














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

const responseBody = "c\r\ndata reached\r\n3\r\nhej\r\n0\r\n\r\n";
const responseBodyDecoded = "data reachedhej";

function contentHandler(metadata, response)
{
  response.seizePower();
  response.write("HTTP/1.1 200 OK\r\n");
  response.write("Content-Type: text/plain\r\n");
  response.write("Transfer-Encoding: chunked\r\n");
  response.write("\r\n");
  response.write(responseBody);
  response.finish();
}

function run_test()
{
  do_get_profile();

  Services.prefs.setIntPref("browser.cache.disk.max_entry_size", 0);

  httpServer = new HttpServer();
  httpServer.registerPathHandler("/content", contentHandler);
  httpServer.start(-1);

  var chan1 = make_channel(URL + "/content");
  chan1.asyncOpen(new ChannelListener(firstTimeThrough, null, CL_ALLOW_UNKNOWN_CL), null);
  var chan2 = make_channel(URL + "/content");
  chan2.asyncOpen(new ChannelListener(secondTimeThrough, null, CL_ALLOW_UNKNOWN_CL), null);

  do_test_pending();
}

function firstTimeThrough(request, buffer)
{
  do_check_eq(buffer, responseBodyDecoded);
}

function secondTimeThrough(request, buffer)
{
  do_check_eq(buffer, responseBodyDecoded);
  httpServer.stop(do_test_finished);
}
