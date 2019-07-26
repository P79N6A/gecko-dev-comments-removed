


























Cu.import("resource://testing-common/httpd.js");
Cu.import("resource://gre/modules/Services.jsm");

XPCOMUtils.defineLazyGetter(this, "URL", function() {
  return "http://localhost:" + httpserver.identity.primaryPort;
});

XPCOMUtils.defineLazyModuleGetter(this, "SafeBrowsing",
  "resource://gre/modules/SafeBrowsing.jsm");

var setCookiePath = "/setcookie";
var checkCookiePath = "/checkcookie";
var safebrowsingUpdatePath = "/safebrowsingUpdate";
var safebrowsingRekeyPath = "/safebrowsingRekey";
var httpserver;

function inChildProcess() {
  return Cc["@mozilla.org/xre/app-info;1"].getService(Ci.nsIXULRuntime)
           .processType != Ci.nsIXULRuntime.PROCESS_TYPE_DEFAULT;
}

function cookieSetHandler(metadata, response) {
  var cookieName = metadata.getHeader("set-cookie");
  response.setStatusLine(metadata.httpVersion, 200, "Ok");
  response.setHeader("set-Cookie", cookieName + "=1; Path=/", false);
  response.setHeader("Content-Type", "text/plain");
  response.bodyOutputStream.write("Ok", "Ok".length);
}

function cookieCheckHandler(metadata, response) {
  var cookies = metadata.getHeader("Cookie");
  response.setStatusLine(metadata.httpVersion, 200, "Ok");
  response.setHeader("saw-cookies", cookies, false);
  response.setHeader("Content-Type", "text/plain");
  response.bodyOutputStream.write("Ok", "Ok".length);
}

function safebrowsingUpdateHandler(metadata, response) {
  var cookieName = "sb-update-cookie";
  response.setStatusLine(metadata.httpVersion, 200, "Ok");
  response.setHeader("set-Cookie", cookieName + "=1; Path=/", false);
  response.setHeader("Content-Type", "text/plain");
  response.bodyOutputStream.write("Ok", "Ok".length);
}

function safebrowsingRekeyHandler(metadata, response) {
  var cookieName = "sb-rekey-cookie";
  response.setStatusLine(metadata.httpVersion, 200, "Ok");
  response.setHeader("Set-Cookie", cookieName + "=1; Path=/", false);
  response.setHeader("Content-Type", "text/plain");
  response.bodyOutputStream.write("Ok", "Ok".length);
}

function setupChannel(path, loadContext) {
  var ios = Cc["@mozilla.org/network/io-service;1"].getService(Ci.nsIIOService);
  var channel = ios.newChannel(URL + path, "", null);
  channel.notificationCallbacks = loadContext;
  channel.QueryInterface(Ci.nsIHttpChannel);
  return channel;
}

function run_test() {

  
  do_get_profile();

  
  if (!inChildProcess())
    Services.prefs.setIntPref("network.cookie.cookieBehavior", 0);

  httpserver = new HttpServer();
  httpserver.registerPathHandler(setCookiePath, cookieSetHandler);
  httpserver.registerPathHandler(checkCookiePath, cookieCheckHandler);
  httpserver.registerPathHandler(safebrowsingUpdatePath, safebrowsingUpdateHandler);
  httpserver.registerPathHandler(safebrowsingRekeyPath, safebrowsingRekeyHandler);

  httpserver.start(-1);
  run_next_test();
}



add_test(function test_safebrowsing_update() {

  var dbservice = Cc["@mozilla.org/url-classifier/dbservice;1"]
                  .getService(Ci.nsIUrlClassifierDBService);
  var streamUpdater = Cc["@mozilla.org/url-classifier/streamupdater;1"]
                     .getService(Ci.nsIUrlClassifierStreamUpdater);

  streamUpdater.updateUrl = URL + safebrowsingUpdatePath;

  function onSuccess() {
    run_next_test();
  }
  function onUpdateError() {
    do_throw("ERROR: received onUpdateError!");
  }
  function onDownloadError() {
    do_throw("ERROR: received onDownloadError!");
  }

  streamUpdater.downloadUpdates("test-phish-simple,test-malware-simple", "",
    "", onSuccess, onUpdateError, onDownloadError);
});



add_test(function test_safebrowsing_rekey() {

  var jslib = Cc["@mozilla.org/url-classifier/jslib;1"]
                .getService().wrappedJSObject;
  var cm = new jslib.PROT_UrlCryptoKeyManager();
  cm.setKeyUrl(URL + safebrowsingRekeyPath);
  cm.reKey();

  run_next_test();
});

add_test(function test_non_safebrowsing_cookie() {

  var cookieName = 'regCookie_id0';
  var loadContext = new LoadContextCallback(0, false, false, false);

  function setNonSafeBrowsingCookie() {
    var channel = setupChannel(setCookiePath, loadContext);
    channel.setRequestHeader("set-cookie", cookieName, false);
    channel.asyncOpen(new ChannelListener(checkNonSafeBrowsingCookie, null), null);
  }

  function checkNonSafeBrowsingCookie() {
    var channel = setupChannel(checkCookiePath, loadContext);
    channel.asyncOpen(new ChannelListener(completeCheckNonSafeBrowsingCookie, null), null);
  }

  function completeCheckNonSafeBrowsingCookie(request, data, context) {
    
    var expectedCookie = cookieName + "=1";
    request.QueryInterface(Ci.nsIHttpChannel);
    var cookiesSeen = request.getResponseHeader("saw-cookies");
    do_check_eq(cookiesSeen, expectedCookie);
    run_next_test();
  }

  setNonSafeBrowsingCookie();
});

add_test(function test_safebrowsing_cookie() {

  var cookieName = 'sbCookie_id4294967294';
  var loadContext = new LoadContextCallback(Ci.nsIScriptSecurityManager.SAFEBROWSING_APP_ID, false, false, false);

  function setSafeBrowsingCookie() {
    var channel = setupChannel(setCookiePath, loadContext);
    channel.setRequestHeader("set-cookie", cookieName, false);
    channel.asyncOpen(new ChannelListener(checkSafeBrowsingCookie, null), null);
  }

  function checkSafeBrowsingCookie() {
    var channel = setupChannel(checkCookiePath, loadContext);
    channel.asyncOpen(new ChannelListener(completeCheckSafeBrowsingCookie, null), null);
  }

  function completeCheckSafeBrowsingCookie(request, data, context) {
    
    
    
    
    var expectedCookies = "sb-update-cookie=1; ";
    expectedCookies += "sb-rekey-cookie=1; ";
    expectedCookies += cookieName + "=1";
    request.QueryInterface(Ci.nsIHttpChannel);
    var cookiesSeen = request.getResponseHeader("saw-cookies");

    do_check_eq(cookiesSeen, expectedCookies);
    httpserver.stop(do_test_finished);
  }

  setSafeBrowsingCookie();
});
