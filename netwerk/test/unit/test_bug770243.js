








Cu.import("resource://testing-common/httpd.js");

var httpserv;

function addCreds(scheme, host)
{
  var authMgr = Components.classes['@mozilla.org/network/http-auth-manager;1']
                          .getService(Ci.nsIHttpAuthManager);
  authMgr.setAuthIdentity(scheme, host, httpserv.identity.primaryPort,
                          "basic", "secret", "/", "", "user", "pass");
}

function clearCreds()
{
  var authMgr = Components.classes['@mozilla.org/network/http-auth-manager;1']
                          .getService(Ci.nsIHttpAuthManager);
  authMgr.clearAll();
}

function makeChan() {
  var ios = Cc["@mozilla.org/network/io-service;1"]
                      .getService(Ci.nsIIOService);
  var chan = ios.newChannel("http://localhost:" +
                            httpserv.identity.primaryPort + "/", null, null)
                .QueryInterface(Ci.nsIHttpChannel);
  return chan;
}



var handlers = [
  
  function(metadata, response) {
    do_check_eq(metadata.hasHeader("Authorization"), false);
    response.setStatusLine(metadata.httpVersion, 200, "OK");
    response.setHeader("ETag", '"one"', false);
    response.setHeader("Cache-control", 'no-cache', false);
    response.setHeader("Content-type", 'text/plain', false);
    var body = "Response body 1";
    response.bodyOutputStream.write(body, body.length);
  },

  
  function(metadata, response) {
    do_check_eq(metadata.hasHeader("Authorization"), false);
    do_check_eq(metadata.getHeader("If-None-Match"), '"one"');
    response.setStatusLine(metadata.httpVersion, 401, "Authenticate");
    response.setHeader("WWW-Authenticate", 'Basic realm="secret"', false);
    addCreds("http", "localhost");
  },
  function(metadata, response) {
    do_check_eq(metadata.hasHeader("Authorization"), true);
    response.setStatusLine(metadata.httpVersion, 200, "OK");
    response.setHeader("ETag", '"two"', false);
    response.setHeader("Cache-control", 'no-cache', false);
    response.setHeader("Content-type", 'text/plain', false);
    var body = "Response body 2";
    response.bodyOutputStream.write(body, body.length);
    clearCreds();
  },

  
  function(metadata, response) {
    do_check_eq(metadata.hasHeader("Authorization"), false);
    do_check_eq(metadata.getHeader("If-None-Match"), '"two"');
    response.setStatusLine(metadata.httpVersion, 401, "Authenticate");
    response.setHeader("WWW-Authenticate", 'Basic realm="secret"', false);
    addCreds("http", "localhost");
  },
  function(metadata, response) {
    do_check_eq(metadata.hasHeader("Authorization"), true);
    do_check_eq(metadata.getHeader("If-None-Match"), '"two"');
    response.setStatusLine(metadata.httpVersion, 304, "OK");
    response.setHeader("ETag", '"two"', false);
    clearCreds();
  },

  
  function(metadata, response) {
    do_check_eq(metadata.hasHeader("Authorization"), false);
    do_check_eq(metadata.getHeader("If-None-Match"), '"two"');
    response.setStatusLine(metadata.httpVersion, 407, "Proxy Authenticate");
    response.setHeader("Proxy-Authenticate", 'Basic realm="secret"', false);
    addCreds("http", "localhost");
  },
  function(metadata, response) {
    do_check_eq(metadata.hasHeader("Proxy-Authorization"), true);
    do_check_eq(metadata.getHeader("If-None-Match"), '"two"');
    response.setStatusLine(metadata.httpVersion, 200, "OK");
    response.setHeader("ETag", '"three"', false);
    response.setHeader("Cache-control", 'no-cache', false);
    response.setHeader("Content-type", 'text/plain', false);
    var body = "Response body 3";
    response.bodyOutputStream.write(body, body.length);
    clearCreds();
  },

  
  function(metadata, response) {
    do_check_eq(metadata.hasHeader("Proxy-Authorization"), false);
    do_check_eq(metadata.getHeader("If-None-Match"), '"three"');
    response.setStatusLine(metadata.httpVersion, 407, "Proxy Authenticate");
    response.setHeader("Proxy-Authenticate", 'Basic realm="secret"', false);
    addCreds("http", "localhost");
  },
  function(metadata, response) {
    do_check_eq(metadata.hasHeader("Proxy-Authorization"), true);
    do_check_eq(metadata.getHeader("If-None-Match"), '"three"');
    response.setStatusLine(metadata.httpVersion, 304, "OK");
    response.setHeader("ETag", '"three"', false);
    response.setHeader("Cache-control", 'no-cache', false);
    clearCreds();
  }
];

function handler(metadata, response)
{
  handlers.shift()(metadata, response);
}



function sync_and_run_next_test()
{
  syncWithCacheIOThread(function() {
    tests.shift()();
  });
}

var tests = [
  
  function() {
    var ch = makeChan();
    ch.asyncOpen(new ChannelListener(function(req, body) {
      do_check_eq(body, "Response body 1");
      sync_and_run_next_test();
    }, null, CL_NOT_FROM_CACHE), null);
  },

  
  function() {
    var ch = makeChan();
    ch.asyncOpen(new ChannelListener(function(req, body) {
      do_check_eq(body, "Response body 2");
      sync_and_run_next_test();
    }, null, CL_NOT_FROM_CACHE), null);
  },

  
  function() {
    var ch = makeChan();
    ch.asyncOpen(new ChannelListener(function(req, body) {
      do_check_eq(body, "Response body 2");
      sync_and_run_next_test();
    }, null, CL_FROM_CACHE), null);
  },

  
  function() {
    var ch = makeChan();
    ch.asyncOpen(new ChannelListener(function(req, body) {
      do_check_eq(body, "Response body 3");
      sync_and_run_next_test();
    }, null, CL_NOT_FROM_CACHE), null);
  },

  
  function() {
    var ch = makeChan();
    ch.asyncOpen(new ChannelListener(function(req, body) {
      do_check_eq(body, "Response body 3");
      sync_and_run_next_test();
    }, null, CL_FROM_CACHE), null);
  },

  
  function() {
    httpserv.stop(do_test_finished);
  }
];

function run_test()
{
  do_get_profile();

  httpserv = new HttpServer();
  httpserv.registerPathHandler("/", handler);
  httpserv.start(-1);

  const prefs = Cc["@mozilla.org/preferences-service;1"]
                         .getService(Ci.nsIPrefBranch);
  prefs.setCharPref("network.proxy.http", "localhost");
  prefs.setIntPref("network.proxy.http_port", httpserv.identity.primaryPort);
  prefs.setCharPref("network.proxy.no_proxies_on", "");
  prefs.setIntPref("network.proxy.type", 1);

  tests.shift()();
  do_test_pending();
}
