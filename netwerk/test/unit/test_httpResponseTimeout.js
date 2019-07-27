





"use strict";

Cu.import("resource://testing-common/httpd.js");

var baseURL;
const kResponseTimeoutPref = "network.http.response.timeout";
const kResponseTimeout = 1;
const kShortLivedKeepalivePref =
  "network.http.tcp_keepalive.short_lived_connections";
const kLongLivedKeepalivePref =
  "network.http.tcp_keepalive.long_lived_connections";

const prefService = Cc["@mozilla.org/preferences-service;1"]
                       .getService(Ci.nsIPrefBranch);

var server = new HttpServer();

function TimeoutListener(expectResponse) {
  this.expectResponse = expectResponse;
}

TimeoutListener.prototype = {
  onStartRequest: function (request, ctx) {
  },

  onDataAvailable: function (request, ctx, stream) {
  },

  onStopRequest: function (request, ctx, status) {
    if (this.expectResponse) {
      do_check_eq(status, Cr.NS_OK);
    } else {
      do_check_eq(status, Cr.NS_ERROR_NET_TIMEOUT);
    }

    run_next_test();
  },
};

function serverStopListener() {
  do_test_finished();
}

function testTimeout(timeoutEnabled, expectResponse) {
  
  if (timeoutEnabled) {
    prefService.setIntPref(kResponseTimeoutPref, kResponseTimeout);
  } else {
    prefService.setIntPref(kResponseTimeoutPref, 0);
  }

  var ios = Cc["@mozilla.org/network/io-service;1"]
  .getService(Ci.nsIIOService);
  var chan = ios.newChannel(baseURL, null, null)
  .QueryInterface(Ci.nsIHttpChannel);
  var listener = new TimeoutListener(expectResponse);
  chan.asyncOpen(listener, null);
}

function testTimeoutEnabled() {
  
  testTimeout(true, false);
}

function testTimeoutDisabled() {
  
  testTimeout(false, true);
}

function testTimeoutDisabledByShortLivedKeepalives() {
  
  prefService.setBoolPref(kShortLivedKeepalivePref, true);
  prefService.setBoolPref(kLongLivedKeepalivePref, false);

  
  testTimeout(true, true);
}

function testTimeoutDisabledByLongLivedKeepalives() {
  
  prefService.setBoolPref(kShortLivedKeepalivePref, false);
  prefService.setBoolPref(kLongLivedKeepalivePref, true);

  
  testTimeout(true, true);
}

function testTimeoutDisabledByBothKeepalives() {
  
  prefService.setBoolPref(kShortLivedKeepalivePref, true);
  prefService.setBoolPref(kLongLivedKeepalivePref, true);

  
  testTimeout(true, true);
}

function setup_tests() {
  
  
  if (prefService.getBoolPref(kShortLivedKeepalivePref)) {
    prefService.setBoolPref(kShortLivedKeepalivePref, false);
    do_register_cleanup(function() {
      prefService.setBoolPref(kShortLivedKeepalivePref, true);
    });
  }
  if (prefService.getBoolPref(kLongLivedKeepalivePref)) {
    prefService.setBoolPref(kLongLivedKeepalivePref, false);
    do_register_cleanup(function() {
      prefService.setBoolPref(kLongLivedKeepalivePref, true);
    });
  }

  var tests = [
    
    testTimeoutEnabled,
    
    testTimeoutDisabled,
    
    testTimeoutDisabledByShortLivedKeepalives,
    
    testTimeoutDisabledByLongLivedKeepalives,
    
    testTimeoutDisabledByBothKeepalives
  ];

  for (var i=0; i < tests.length; i++) {
    add_test(tests[i]);
  }
}

function setup_http_server() {
  
  server.start(-1);
  baseURL = "http://localhost:" + server.identity.primaryPort + "/";
  do_print("Using baseURL: " + baseURL);
  server.registerPathHandler('/', function(metadata, response) {
    
    response.processAsync();

    do_timeout((kResponseTimeout+1)*1000 , function() {
      response.setStatusLine(metadata.httpVersion, 200, "OK");
      response.write("Hello world");
      response.finish();
    });
  });
  do_register_cleanup(function() {
    server.stop(serverStopListener);
  });
}

function run_test() {
  setup_http_server();

  setup_tests();

  run_next_test();
}
