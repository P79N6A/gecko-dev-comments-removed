Cu.import("resource://testing-common/httpd.js");
Cu.import("resource://gre/modules/Services.jsm");

XPCOMUtils.defineLazyGetter(this, "URL", function() {
  return "http://localhost:" + httpserver.identity.primaryPort;
});

var httpserver = new HttpServer();
var testpath = "/simple";
var httpbody = "0123456789";

var live_channels = [];

function run_test() {
  httpserver.registerPathHandler(testpath, serverHandler);
  httpserver.start(-1);

  var local_channel;

  
  local_channel = setupChannel(testpath);
  local_channel.asyncOpen(
      new ChannelListener(checkRequest, local_channel), null);

  
  setupChannel(testpath).asyncOpen(
      new ChannelListener(function() {}, null), null);
  
  
  live_channels.push(setupChannel(testpath));

  
  live_channels.push(setupChannel(testpath));
  live_channels[1].asyncOpen(
      new ChannelListener(checkRequestFinish, live_channels[1]), null);

  do_test_pending();
}

function setupChannel(path) {
  var ios = Cc["@mozilla.org/network/io-service;1"].getService(Ci.nsIIOService);
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

function serverHandler(metadata, response) {
  response.setHeader("Content-Type", "text/plain", false);
  response.bodyOutputStream.write(httpbody, httpbody.length);
}

function checkRequest(request, data, context) {
  do_check_eq(data, httpbody);
}

function checkRequestFinish(request, data, context) {
  checkRequest(request, data, context);
  httpserver.stop(do_test_finished);
}
