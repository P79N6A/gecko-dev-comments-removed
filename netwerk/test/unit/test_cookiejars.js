







 

XPCOMUtils.defineLazyGetter(this, "URL", function() {
  return "http://localhost:" + httpserver.identity.primaryPort;
});

Cu.import("resource://testing-common/httpd.js");
Cu.import("resource://gre/modules/Services.jsm");
var httpserver = new HttpServer();

var cookieSetPath = "/setcookie";
var cookieCheckPath = "/checkcookie";

function inChildProcess() {
  return Cc["@mozilla.org/xre/app-info;1"]
           .getService(Ci.nsIXULRuntime)
           .processType != Ci.nsIXULRuntime.PROCESS_TYPE_DEFAULT;
}








var tests = [
  { cookieName: 'LCC_App0_BrowF_PrivF', 
    loadContext: new LoadContextCallback(0, false, false, 1) }, 
  { cookieName: 'LCC_App0_BrowT_PrivF', 
    loadContext: new LoadContextCallback(0, true,  false, 1) }, 
  { cookieName: 'LCC_App1_BrowF_PrivF', 
    loadContext: new LoadContextCallback(1, false, false, 1) }, 
  { cookieName: 'LCC_App1_BrowT_PrivF', 
    loadContext: new LoadContextCallback(1, true,  false, 1) }, 
];


var i = 0;

function setupChannel(path)
{
  var ios = Cc["@mozilla.org/network/io-service;1"].getService(Ci.nsIIOService);
  var chan = ios.newChannel(URL + path, "", null);
  chan.notificationCallbacks = tests[i].loadContext;
  chan.QueryInterface(Ci.nsIHttpChannel);
  return chan;
}

function setCookie() {
  var channel = setupChannel(cookieSetPath);
  channel.setRequestHeader("foo-set-cookie", tests[i].cookieName, false);
  channel.asyncOpen(new ChannelListener(setNextCookie, null), null);
}

function setNextCookie(request, data, context) 
{
  if (++i == tests.length) {
    
    i = 0;
    checkCookie();
  } else {
    do_print("setNextCookie:i=" + i);
    setCookie();
  }
}



function checkCookie()
{
  var channel = setupChannel(cookieCheckPath);
  channel.asyncOpen(new ChannelListener(completeCheckCookie, null), null);
}

function completeCheckCookie(request, data, context) {
  
  
  var expectedCookie = tests[i].cookieName;
  request.QueryInterface(Ci.nsIHttpChannel);
  var cookiesSeen = request.getResponseHeader("foo-saw-cookies");

  var j;
  for (j = 0; j < tests.length; j++) {
    var cookieToCheck = tests[j].cookieName;
    found = (cookiesSeen.indexOf(cookieToCheck) != -1);
    if (found && expectedCookie != cookieToCheck) {
      do_throw("test index " + i + ": found unexpected cookie '" 
          + cookieToCheck + "': in '" + cookiesSeen + "'");
    } else if (!found && expectedCookie == cookieToCheck) {
      do_throw("test index " + i + ": missing expected cookie '" 
          + expectedCookie + "': in '" + cookiesSeen + "'");
    }
  }
  
  do_print("Saw only correct cookie '" + expectedCookie + "'");
  do_check_true(true);


  if (++i == tests.length) {
    
    httpserver.stop(do_test_finished);
  } else {
    checkCookie();
  }
}

function run_test()
{
  
  if (!inChildProcess())
    Services.prefs.setIntPref("network.cookie.cookieBehavior", 0);

  httpserver.registerPathHandler(cookieSetPath, cookieSetHandler);
  httpserver.registerPathHandler(cookieCheckPath, cookieCheckHandler);
  httpserver.start(-1);

  setCookie();
  do_test_pending();
}

function cookieSetHandler(metadata, response)
{
  var cookieName = metadata.getHeader("foo-set-cookie");

  response.setStatusLine(metadata.httpVersion, 200, "Ok");
  response.setHeader("Set-Cookie", cookieName + "=1; Path=/", false);
  response.setHeader("Content-Type", "text/plain");
  response.bodyOutputStream.write("Ok", "Ok".length);
}

function cookieCheckHandler(metadata, response)
{
  var cookies = metadata.getHeader("Cookie");

  response.setStatusLine(metadata.httpVersion, 200, "Ok");
  response.setHeader("foo-saw-cookies", cookies, false);
  response.setHeader("Content-Type", "text/plain");
  response.bodyOutputStream.write("Ok", "Ok".length);
}

