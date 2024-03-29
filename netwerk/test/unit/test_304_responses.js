"use strict";


Cu.import("resource://testing-common/httpd.js");
Cu.import("resource://gre/modules/Services.jsm");

XPCOMUtils.defineLazyGetter(this, "URL", function() {
  return "http://localhost:" + httpServer.identity.primaryPort;
});

var httpServer = null;
const testFileName = "test_customConditionalRequest_304";
const basePath = "/" + testFileName + "/";

XPCOMUtils.defineLazyGetter(this, "baseURI", function() {
  return URL + basePath;
});

const unexpected304 = "unexpected304";
const existingCached304 = "existingCached304";

function make_uri(url) {
  var ios = Cc["@mozilla.org/network/io-service;1"].
            getService(Ci.nsIIOService);
  return ios.newURI(url, null, null);
}

function make_channel(url) {
  var ios = Cc["@mozilla.org/network/io-service;1"].getService(Ci.nsIIOService);
  var chan = ios.newChannel2(url,
                             null,
                             null,
                             null,      
                             Services.scriptSecurityManager.getSystemPrincipal(),
                             null,      
                             Ci.nsILoadInfo.SEC_NORMAL,
                             Ci.nsIContentPolicy.TYPE_OTHER).QueryInterface(Ci.nsIHttpChannel);
  return chan;
}

function clearCache() {
    var service = Components.classes["@mozilla.org/netwerk/cache-storage-service;1"]
        .getService(Ci.nsICacheStorageService);
    service.clear();
}

function alwaysReturn304Handler(metadata, response) {
  response.setStatusLine(metadata.httpVersion, 304, "Not Modified");
  response.setHeader("Returned-From-Handler", "1");
}

function run_test() {
  evict_cache_entries();

  httpServer = new HttpServer();
  httpServer.registerPathHandler(basePath + unexpected304,
                                 alwaysReturn304Handler);
  httpServer.registerPathHandler(basePath + existingCached304,
                                 alwaysReturn304Handler);
  httpServer.start(-1);
  run_next_test();
}

function finish_test(request, buffer) {
  httpServer.stop(do_test_finished);
}

function consume304(request, buffer) {
  request.QueryInterface(Components.interfaces.nsIHttpChannel);
  do_check_eq(request.responseStatus, 304);
  do_check_eq(request.getResponseHeader("Returned-From-Handler"), "1");
  run_next_test();
}



add_test(function test_unexpected_304() {
  var chan = make_channel(baseURI + unexpected304);
  chan.asyncOpen(new ChannelListener(consume304, null), null);
});



add_test(function test_304_stored_in_cache() {
  asyncOpenCacheEntry(
    baseURI + existingCached304, "disk", Ci.nsICacheStorage.OPEN_NORMALLY, null,
    function (entryStatus, cacheEntry) {
      cacheEntry.setMetaDataElement("request-method", "GET");
      cacheEntry.setMetaDataElement("response-head",
                                    "HTTP/1.1 304 Not Modified\r\n" +
                                    "\r\n");
      cacheEntry.metaDataReady();
      cacheEntry.close();

      var chan = make_channel(baseURI + existingCached304);

      
      chan.QueryInterface(Components.interfaces.nsIHttpChannel);
      chan.setRequestHeader("If-None-Match", '"foo"', false);

      chan.asyncOpen(new ChannelListener(consume304, null), null);
    });
});
