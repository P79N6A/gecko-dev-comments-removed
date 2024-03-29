Cu.import("resource://testing-common/httpd.js");
Cu.import("resource://gre/modules/Services.jsm");

XPCOMUtils.defineLazyGetter(this, "URL", function() {
  return "http://localhost:" + httpserver.identity.primaryPort;
});

var httpserver = new HttpServer();
var testpath = "/421";
var httpbody = "0123456789";
var channel;
var ios;

function run_test() {
  setup_test();
  do_test_pending();
}

function setup_test() {
  httpserver.registerPathHandler(testpath, serverHandler);
  httpserver.start(-1);

  channel = setupChannel(testpath);

  channel.asyncOpen(new ChannelListener(checkRequestResponse, channel), null);
}

function setupChannel(path) {
  ios = Cc["@mozilla.org/network/io-service;1"].getService(Ci.nsIIOService);
  var chan = ios.newChannel2(URL + path,
                             "",
                             null,
                             null,      
                             Services.scriptSecurityManager.getSystemPrincipal(),
                             null,      
                             Ci.nsILoadInfo.SEC_NORMAL,
                             Ci.nsIContentPolicy.TYPE_OTHER);
  chan.QueryInterface(Ci.nsIHttpChannel);
  chan.requestMethod = "GET";
  return chan;
}

var iters = 0;

function serverHandler(metadata, response) {
  response.setHeader("Content-Type", "text/plain", false);

  if (!iters) {
    response.setStatusLine("1.1", 421, "Not Authoritative " + iters);
  } else {
    response.setStatusLine("1.1", 200, "OK");
  }
  ++iters;

  response.bodyOutputStream.write(httpbody, httpbody.length);
}

function checkRequestResponse(request, data, context) {
  do_check_eq(channel.responseStatus, 200);
  do_check_eq(channel.responseStatusText, "OK");
  do_check_true(channel.requestSucceeded);

  do_check_eq(channel.contentType, "text/plain");
  do_check_eq(channel.contentLength, httpbody.length);
  do_check_eq(data, httpbody);

  httpserver.stop(do_test_finished);
}
