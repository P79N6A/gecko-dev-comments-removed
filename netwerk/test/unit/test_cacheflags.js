const Cc = Components.classes;
const Ci = Components.interfaces;
const Cu = Components.utils;
const Cr = Components.results;

Cu.import("resource://testing-common/httpd.js");

var httpserver = new HttpServer();
httpserver.start(-1);


var suffix = Math.random();
var httpBase = "http://localhost:" + httpserver.identity.primaryPort;
var httpsBase = "http://localhost:4445";
var shortexpPath = "/shortexp" + suffix;
var longexpPath = "/longexp/" + suffix;
var longexp2Path = "/longexp/2/" + suffix;
var nocachePath = "/nocache" + suffix;
var nostorePath = "/nostore" + suffix;


function LoadContext(usePrivateBrowsing) {
  this.usePrivateBrowsing = usePrivateBrowsing;
}

LoadContext.prototype = {
  usePrivateBrowsing: false,
  

  QueryInterface: function(iid) {
    if (iid.equals(Ci.nsILoadContext))
      return this;
    throw Cr.NS_ERROR_NO_INTERFACE;
  },

  getInterface: function(iid) {
    if (iid.equals(Ci.nsILoadContext))
      return this;
    throw Cr.NS_ERROR_NO_INTERFACE;
  }
};

PrivateBrowsingLoadContext = new LoadContext(true);

function make_channel(url, flags, usePrivateBrowsing) {
  var ios = Cc["@mozilla.org/network/io-service;1"].
    getService(Ci.nsIIOService);
  var req = ios.newChannel(url, null, null);
  req.loadFlags = flags;
  if (usePrivateBrowsing) {
    req.notificationCallbacks = PrivateBrowsingLoadContext;    
  }
  return req;
}

function Test(path, flags, expectSuccess, readFromCache, hitServer, 
              usePrivateBrowsing ) {
  this.path = path;
  this.flags = flags;
  this.expectSuccess = expectSuccess;
  this.readFromCache = readFromCache;
  this.hitServer = hitServer;
  this.usePrivateBrowsing = usePrivateBrowsing;
}

Test.prototype = {
  flags: 0,
  expectSuccess: true,
  readFromCache: false,
  hitServer: true,
  usePrivateBrowsing: false,
  _buffer: "",
  _isFromCache: false,

  QueryInterface: function(iid) {
    if (iid.equals(Components.interfaces.nsIStreamListener) ||
        iid.equals(Components.interfaces.nsIRequestObserver) ||
        iid.equals(Components.interfaces.nsISupports))
      return this;
    throw Components.results.NS_ERROR_NO_INTERFACE;
  },

  onStartRequest: function(request, context) {
    var cachingChannel = request.QueryInterface(Ci.nsICacheInfoChannel);
    this._isFromCache = request.isPending() && cachingChannel.isFromCache();
  },

  onDataAvailable: function(request, context, stream, offset, count) {
    this._buffer = this._buffer.concat(read_stream(stream, count));
  },

  onStopRequest: function(request, context, status) {
    do_check_eq(Components.isSuccessCode(status), this.expectSuccess);
    do_check_eq(this._isFromCache, this.readFromCache);
    do_check_eq(gHitServer, this.hitServer);

    do_timeout(0, run_next_test);
  },

  run: function() {
    dump("Running:" +
         "\n  " + this.path +
         "\n  " + this.flags +
         "\n  " + this.expectSuccess +
         "\n  " + this.readFromCache +
         "\n  " + this.hitServer + "\n");
    gHitServer = false;
    var channel = make_channel(this.path, this.flags, this.usePrivateBrowsing);
    channel.asyncOpen(this, null);
  }
};

var gHitServer = false;

var gTests = [

  new Test(httpBase + shortexpPath, 0,
           true,   
           false,  
           true,   
           true),  
  new Test(httpBase + shortexpPath, 0,
           true,   
           false,  
           true),  
  new Test(httpBase + shortexpPath, 0,
           true,   
           true,   
           true),  
  new Test(httpBase + shortexpPath, Ci.nsIRequest.LOAD_BYPASS_CACHE,
           true,   
           false,  
           true),  
  new Test(httpBase + shortexpPath, Ci.nsICachingChannel.LOAD_ONLY_FROM_CACHE,
           false,  
           false,  
           false), 
  new Test(httpBase + shortexpPath,
           Ci.nsICachingChannel.LOAD_ONLY_FROM_CACHE |
           Ci.nsIRequest.VALIDATE_NEVER,
           true,   
           true,   
           false), 
  new Test(httpBase + shortexpPath, Ci.nsIRequest.LOAD_FROM_CACHE,
           true,   
           true,   
           false), 

  new Test(httpBase + longexpPath, 0,
           true,   
           false,  
           true),  
  new Test(httpBase + longexpPath, 0,
           true,   
           true,   
           false), 
  new Test(httpBase + longexpPath, Ci.nsIRequest.LOAD_BYPASS_CACHE,
           true,   
           false,  
           true),  
  new Test(httpBase + longexpPath,
           Ci.nsIRequest.VALIDATE_ALWAYS,
           true,   
           true,   
           true),  
  new Test(httpBase + longexpPath, Ci.nsICachingChannel.LOAD_ONLY_FROM_CACHE,
           true,   
           true,   
           false), 
  new Test(httpBase + longexpPath,
           Ci.nsICachingChannel.LOAD_ONLY_FROM_CACHE |
           Ci.nsIRequest.VALIDATE_NEVER,
           true,   
           true,   
           false), 
  new Test(httpBase + longexpPath,
           Ci.nsICachingChannel.LOAD_ONLY_FROM_CACHE |
           Ci.nsIRequest.VALIDATE_ALWAYS,
           false,  
           false,  
           false), 
  new Test(httpBase + longexpPath, Ci.nsIRequest.LOAD_FROM_CACHE,
           true,   
           true,   
           false), 

  new Test(httpBase + longexp2Path, 0,
           true,   
           false,  
           true),  
  new Test(httpBase + longexp2Path, 0,
           true,   
           true,   
           false), 

  new Test(httpBase + nocachePath, 0,
           true,   
           false,  
           true),  
  new Test(httpBase + nocachePath, 0,
           true,   
           true,   
           true),  
  new Test(httpBase + nocachePath, Ci.nsICachingChannel.LOAD_ONLY_FROM_CACHE,
           false,  
           false,  
           false), 
  new Test(httpBase + nocachePath, Ci.nsIRequest.LOAD_FROM_CACHE,
           true,   
           true,   
           false), 
  
  
  new Test(httpBase + nocachePath,
           Ci.nsICachingChannel.LOAD_ONLY_FROM_CACHE |
           Ci.nsIRequest.VALIDATE_NEVER,
           true,   
           true,   
           false), 

  
  
  
  








  new Test(httpBase + nostorePath, 0,
           true,   
           false,  
           true),  
  new Test(httpBase + nostorePath, 0,
           true,   
           false,  
           true),  
  new Test(httpBase + nostorePath, Ci.nsICachingChannel.LOAD_ONLY_FROM_CACHE,
           false,  
           false,  
           false), 
  new Test(httpBase + nostorePath, Ci.nsIRequest.LOAD_FROM_CACHE,
           true,   
           true,   
           false), 
  
  
  new Test(httpBase + nostorePath,
           Ci.nsICachingChannel.LOAD_ONLY_FROM_CACHE |
           Ci.nsIRequest.VALIDATE_NEVER,
           false,  
           false,  
           false)  
  ];

function run_next_test()
{
  if (gTests.length == 0) {
    httpserver.stop(do_test_finished);
    return;
  }

  var test = gTests.shift();
  test.run();
}

function handler(metadata, response) {
  gHitServer = true;
  try {
    var etag = metadata.getHeader("If-None-Match");
  } catch(ex) {
    var etag = "";
  }
  if (etag == "testtag") {
    
    response.setStatusLine(metadata.httpVersion, 304, "Not Modified");
  } else {
    response.setStatusLine(metadata.httpVersion, 200, "OK");
    response.setHeader("Content-Type", "text/plain", false);
    response.setHeader("ETag", "testtag", false);
    const body = "data";
    response.bodyOutputStream.write(body, body.length);
  }
}

function nocache_handler(metadata, response) {
  response.setHeader("Cache-Control", "no-cache", false);
  handler(metadata, response);
}

function nostore_handler(metadata, response) {
  response.setHeader("Cache-Control", "no-store", false);
  handler(metadata, response);
}

function shortexp_handler(metadata, response) {
  response.setHeader("Cache-Control", "max-age=0", false);
  handler(metadata, response);
}

function longexp_handler(metadata, response) {
  response.setHeader("Cache-Control", "max-age=10000", false);
  handler(metadata, response);
}


function longexp2_handler(metadata, response) {
  response.setHeader("Cache-Control", "max-age = 10000", false);
  handler(metadata, response);
}

function run_test() {
  httpserver.registerPathHandler(shortexpPath, shortexp_handler);
  httpserver.registerPathHandler(longexpPath, longexp_handler);
  httpserver.registerPathHandler(longexp2Path, longexp2_handler);
  httpserver.registerPathHandler(nocachePath, nocache_handler);
  httpserver.registerPathHandler(nostorePath, nostore_handler);

  run_next_test();
  do_test_pending();
}
