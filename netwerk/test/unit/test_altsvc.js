Cu.import("resource://testing-common/httpd.js");
Cu.import("resource://gre/modules/Services.jsm");
Cu.import("resource://gre/modules/NetUtil.jsm");

var h2Port;
var prefs;
var spdypref;
var http2pref;
var tlspref;
var altsvcpref1;
var altsvcpref2;



var h1Foo; 
var h1Bar; 

var h2FooRoute; 
var h2BarRoute; 
var h2Route;    
var httpFooOrigin; 
var httpsFooOrigin; 
var httpBarOrigin; 
var httpsBarOrigin; 

function run_test() {
  var env = Cc["@mozilla.org/process/environment;1"].getService(Ci.nsIEnvironment);
  h2Port = env.get("MOZHTTP2-PORT");
  do_check_neq(h2Port, null);
  do_check_neq(h2Port, "");

  
  do_get_profile();
  prefs = Cc["@mozilla.org/preferences-service;1"].getService(Ci.nsIPrefBranch);

  spdypref = prefs.getBoolPref("network.http.spdy.enabled");
  http2pref = prefs.getBoolPref("network.http.spdy.enabled.http2");
  tlspref = prefs.getBoolPref("network.http.spdy.enforce-tls-profile");
  altsvcpref1 = prefs.getBoolPref("network.http.altsvc.enabled");
  altsvcpref2 = prefs.getBoolPref("network.http.altsvc.oe", true);

  prefs.setBoolPref("network.http.spdy.enabled", true);
  prefs.setBoolPref("network.http.spdy.enabled.http2", true);
  prefs.setBoolPref("network.http.spdy.enforce-tls-profile", false);
  prefs.setBoolPref("network.http.altsvc.enabled", true);
  prefs.setBoolPref("network.http.altsvc.oe", true);
  prefs.setCharPref("network.dns.localDomains", "foo.example.com, bar.example.com");

  
  
  
  
  let certdb = Cc["@mozilla.org/security/x509certdb;1"]
                  .getService(Ci.nsIX509CertDB);
  addCertFromFile(certdb, "CA.cert.der", "CTu,u,u");

  h1Foo = new HttpServer();
  h1Foo.registerPathHandler("/altsvc-test", h1Server);
  h1Foo.start(-1);
  h1Foo.identity.setPrimary("http", "foo.example.com", h1Foo.identity.primaryPort);

  h1Bar = new HttpServer();
  h1Bar.registerPathHandler("/altsvc-test", h1Server);
  h1Bar.start(-1);
  h1Bar.identity.setPrimary("http", "bar.example.com", h1Bar.identity.primaryPort);

  h2FooRoute = "foo.example.com:" + h2Port;
  h2BarRoute = "bar.example.com:" + h2Port;
  h2Route = ":" + h2Port;

  httpFooOrigin = "http://foo.example.com:" + h1Foo.identity.primaryPort + "/";
  httpsFooOrigin = "https://" + h2FooRoute + "/";
  httpBarOrigin = "http://bar.example.com:" + h1Bar.identity.primaryPort + "/";
  httpsBarOrigin = "https://" + h2BarRoute + "/";
  dump ("http foo - " + httpFooOrigin + "\n" +
        "https foo - " + httpsFooOrigin + "\n" +
        "http bar - " + httpBarOrigin + "\n" +
        "https bar - " + httpsBarOrigin + "\n");

  doTest1();
}

function h1Server(metadata, response) {
  response.setStatusLine(metadata.httpVersion, 200, "OK");
  response.setHeader("Content-Type", "text/plain", false);
  response.setHeader("Connection", "close", false);
  response.setHeader("Cache-Control", "no-cache", false);
  response.setHeader("Access-Control-Allow-Origin", "*", false);
  response.setHeader("Access-Control-Allow-Method", "GET", false);
  response.setHeader("Access-Control-Allow-Headers", "x-altsvc", false);

  try {
    var hval = "h2=" + metadata.getHeader("x-altsvc");
    response.setHeader("Alt-Svc", hval, false);
  } catch (e) {}

  var body = "Q: What did 0 say to 8? A: Nice Belt!\n";
  response.bodyOutputStream.write(body, body.length);
}

function resetPrefs() {
  prefs.setBoolPref("network.http.spdy.enabled", spdypref);
  prefs.setBoolPref("network.http.spdy.enabled.http2", http2pref);
  prefs.setBoolPref("network.http.spdy.enforce-tls-profile", tlspref);
  prefs.setBoolPref("network.http.altsvc.enabled", altsvcpref1);
  prefs.setBoolPref("network.http.altsvc.oe", altsvcpref2);
  prefs.clearUserPref("network.dns.localDomains");
}

function readFile(file) {
  let fstream = Cc["@mozilla.org/network/file-input-stream;1"]
                  .createInstance(Ci.nsIFileInputStream);
  fstream.init(file, -1, 0, 0);
  let data = NetUtil.readInputStreamToString(fstream, fstream.available());
  fstream.close();
  return data;
}

function addCertFromFile(certdb, filename, trustString) {
  let certFile = do_get_file(filename, false);
  let der = readFile(certFile);
  certdb.addCert(der, trustString, null);
}

function makeChan(origin) {
  var ios = Cc["@mozilla.org/network/io-service;1"].getService(Ci.nsIIOService);
  var chan = ios.newChannel2(origin + "altsvc-test",
                             null,
                             null,
                             null,      
                             Services.scriptSecurityManager.getSystemPrincipal(),
                             null,      
                             Ci.nsILoadInfo.SEC_NORMAL,
                             Ci.nsIContentPolicy.TYPE_OTHER).QueryInterface(Ci.nsIHttpChannel);

  return chan;
}

var origin;
var xaltsvc;
var retryCounter = 0;
var loadWithoutAltSvc = false;
var nextTest;
var expectPass = true;
var waitFor = 0;

var Listener = function() {};
Listener.prototype = {
  onStartRequest: function testOnStartRequest(request, ctx) {
    do_check_true(request instanceof Components.interfaces.nsIHttpChannel);

    if (expectPass) {
      if (!Components.isSuccessCode(request.status)) {
        do_throw("Channel should have a success code! (" + request.status + ")");
      }
      do_check_eq(request.responseStatus, 200);
    } else {
      do_check_eq(Components.isSuccessCode(request.status), false);
    }
  },

  onDataAvailable: function testOnDataAvailable(request, ctx, stream, off, cnt) {
    read_stream(stream, cnt);
  },

  onStopRequest: function testOnStopRequest(request, ctx, status) {
    var routed = "";
    try {
      routed = request.getRequestHeader("Alt-Used");
    } catch (e) {}
    dump("routed is " + routed + "\n");

    if (waitFor != 0) {
      do_check_eq(routed, "");
      do_test_pending();
      do_timeout(waitFor, doTest);
      waitFor = 0;
      xaltsvc = "NA";
    } else if (xaltsvc == "NA") {
      do_check_eq(routed, "");
      nextTest();
    } else if (routed == xaltsvc) {
      do_check_eq(routed, xaltsvc); 
      nextTest();
    } else {
      dump ("poll later for alt svc mapping\n");
      do_test_pending();
      do_timeout(500, doTest);
    }

    do_test_finished();
  }
};

function testsDone()
{
  dump("testDone\n");
  resetPrefs();
  do_test_pending();
  h1Foo.stop(do_test_finished);
  do_test_pending();
  h1Bar.stop(do_test_finished);
}

function doTest()
{
  dump("execute doTest " + origin + "\n");
  var chan = makeChan(origin);
  var listener = new Listener();
  if (xaltsvc != "NA") {
    chan.setRequestHeader("x-altsvc", xaltsvc, false);
  }
  chan.loadFlags = Ci.nsIRequest.LOAD_FRESH_CONNECTION |
	           Ci.nsIChannel.LOAD_INITIAL_DOCUMENT_URI;
  chan.asyncOpen(listener, null);
}








    

function doTest1()
{
  dump("doTest1()\n");
  origin = httpFooOrigin;
  xaltsvc = h2Route;
  nextTest = doTest2;
  do_test_pending();
  doTest();
  xaltsvc = h2FooRoute;
}


function doTest2()
{
  dump("doTest2()\n");
  origin = httpFooOrigin;
  xaltsvc = h2FooRoute;
  nextTest = doTest3;
  do_test_pending();
  doTest();
}



function doTest3()
{
  dump("doTest3()\n");
  origin = httpFooOrigin;
  xaltsvc = h2BarRoute;
  nextTest = doTest4;
  do_test_pending();
  doTest();
}


function doTest4()
{
  dump("doTest4()\n");
  origin = httpsBarOrigin;
  xaltsvc = '';
  expectPass = false;
  nextTest = doTest5;
  do_test_pending();
  doTest();
}


function doTest5()
{
  dump("doTest5()\n");
  origin = httpsFooOrigin;
  xaltsvc = 'NA';
  expectPass = true;
  nextTest = doTest6;
  do_test_pending();
  doTest();
}


function doTest6()
{
  dump("doTest6()\n");
  origin = httpsFooOrigin;
  xaltsvc = h2BarRoute;
  nextTest = doTest7;
  do_test_pending();
  doTest();
}


function doTest7()
{
  dump("doTest7()\n");
  origin = httpsBarOrigin;
  xaltsvc = '';
  expectPass = false;
  nextTest = doTest8;
  do_test_pending();
  doTest();
}


function doTest8()
{
  dump("doTest8()\n");
  origin = httpBarOrigin;
  xaltsvc = h2BarRoute;
  expectPass = true;
  nextTest = doTest9;
  do_test_pending();
  doTest();
}


function doTest9()
{
  dump("doTest9()\n");
  origin = httpBarOrigin;
  xaltsvc = h2Route;
  nextTest = doTest10;
  do_test_pending();
  doTest();
  xaltsvc = h2BarRoute;
}


function doTest10()
{
  dump("doTest10()\n");
  origin = httpsBarOrigin;
  xaltsvc = '';
  expectPass = false;
  nextTest = doTest11;
  do_test_pending();
  doTest();
}




function doTest11()
{
  dump("doTest11()\n");
  origin = httpBarOrigin;
  xaltsvc = h2FooRoute;
  expectPass = true;
  waitFor = 1000;
  nextTest = testsDone;
  do_test_pending();
  doTest();
}

