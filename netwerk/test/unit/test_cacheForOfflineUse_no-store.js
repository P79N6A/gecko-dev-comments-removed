"use strict";


const Cc = Components.classes;
const Ci = Components.interfaces;
const Cu = Components.utils;
const Cr = Components.results;

Cu.import("resource://testing-common/httpd.js");

var httpServer = null;
const testFileName = "test_nsHttpChannel_CacheForOfflineUse-no-store";
const cacheClientID = testFileName + "|fake-group-id";
const basePath = "/" + testFileName + "/";

XPCOMUtils.defineLazyGetter(this, "baseURI", function() {
  return "http://localhost:" + httpServer.identity.primaryPort + basePath;
});

const normalEntry = "normal";
const noStoreEntry = "no-store";

var cacheUpdateObserver = null;

function make_channel_for_offline_use(url, callback, ctx) {
  var ios = Cc["@mozilla.org/network/io-service;1"].
            getService(Ci.nsIIOService);
  var chan = ios.newChannel(url, "", null);
  
  var cacheService = Components.classes["@mozilla.org/network/application-cache-service;1"].
                     getService(Components.interfaces.nsIApplicationCacheService);
  var appCache = cacheService.getApplicationCache(cacheClientID);
  
  var appCacheChan = chan.QueryInterface(Ci.nsIApplicationCacheChannel);
  appCacheChan.applicationCacheForWrite = appCache;
  return chan;
}

function make_uri(url) {
  var ios = Cc["@mozilla.org/network/io-service;1"].
            getService(Ci.nsIIOService);
  return ios.newURI(url, null, null);
}

function CacheListener() { }
CacheListener.prototype = {
  QueryInterface : function(iid)
  {
    if (iid.equals(Components.interfaces.nsICacheListener))
      return this;
    throw Components.results.NS_NOINTERFACE;
  },
};


function asyncCheckCacheEntryExistance(entryName, shouldExist)
{
  var listener = new CacheListener();
  listener.onCacheEntryAvailable = function(descriptor, accessGranted, status) {
    if (shouldExist) {
      do_check_eq(status, Cr.NS_OK);
      do_check_true(!!descriptor);
    } else {
      do_check_eq(status, Cr.NS_ERROR_CACHE_KEY_NOT_FOUND);
      do_check_null(descriptor);
    }
    run_next_test();
  };

  var service = Cc["@mozilla.org/network/cache-service;1"]
                          .getService(Ci.nsICacheService);
  var session = service.createSession(cacheClientID, Ci.nsICache.STORE_OFFLINE,
                                        true);
  session.asyncOpenCacheEntry(baseURI + entryName, Ci.nsICache.ACCESS_READ,
                              listener);
}

const responseBody = "response body";


function normalHandler(metadata, response)
{
  do_print("normalHandler");
  response.setHeader("Content-Type", "text/plain");
  response.bodyOutputStream.write(responseBody, responseBody.length);
}
function checkNormal(request, buffer)
{
  do_check_eq(buffer, responseBody);
  asyncCheckCacheEntryExistance(normalEntry, true);
}
add_test(function test_normal() {
  var chan = make_channel_for_offline_use(baseURI + normalEntry);
  chan.asyncOpen(new ChannelListener(checkNormal, chan), null);
});



function noStoreHandler(metadata, response)
{
  do_print("noStoreHandler");
  response.setHeader("Content-Type", "text/plain");
  response.setHeader("Cache-Control", "no-store");
  response.bodyOutputStream.write(responseBody, responseBody.length);
}
function checkNoStore(request, buffer)
{
  do_check_eq(buffer, "");
  asyncCheckCacheEntryExistance(noStoreEntry, false);
  run_next_test();
}
add_test(function test_noStore() {
  var chan = make_channel_for_offline_use(baseURI + noStoreEntry);
  
  chan.asyncOpen(new ChannelListener(checkNoStore, chan, CL_EXPECT_FAILURE),
                 null);
});

function run_test()
{
  do_get_profile();

  httpServer = new HttpServer();
  httpServer.registerPathHandler(basePath + normalEntry, normalHandler);
  httpServer.registerPathHandler(basePath + noStoreEntry, noStoreHandler);
  httpServer.start(-1);
  run_next_test();
}

function finish_test(request, buffer)
{
  httpServer.stop(do_test_finished);
}
