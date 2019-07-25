
















var firstTest = 1;   
var lastTest = 4;    



const Cc = Components.classes;
const Ci = Components.interfaces;
const Cu = Components.utils;
const Cr = Components.results;

Cu.import("resource://testing-common/httpd.js");

var httpserver = new HttpServer();
var index = 0;
var nextTest = firstTest;
var test_flags = new Array();
var testPathBase = "/test_headers";

function run_test()
{
  httpserver.start(4444);

  do_test_pending();
  run_test_number(nextTest);
}

function runNextTest()
{
  if (nextTest == lastTest) {
    endTests();
    return;
  }
  nextTest++;
  
  if (eval("handler" + nextTest) == undefined)
    do_throw("handler" + nextTest + " undefined!");
  if (eval("completeTest" + nextTest) == undefined)
    do_throw("completeTest" + nextTest + " undefined!");
  
  run_test_number(nextTest);
}

function run_test_number(num)
{
  testPath = testPathBase + num;
  httpserver.registerPathHandler(testPath, eval("handler" + num));

  var channel = setupChannel(testPath);
  flags = test_flags[num];   
  channel.asyncOpen(new ChannelListener(eval("completeTest" + num),
                                        channel, flags), null);
}

function setupChannel(url)
{
  var ios = Components.classes["@mozilla.org/network/io-service;1"].
                       getService(Ci.nsIIOService);
  var chan = ios.newChannel("http://localhost:4444" + url, "", null);
  var httpChan = chan.QueryInterface(Components.interfaces.nsIHttpChannel);
  return httpChan;
}

function endTests()
{
  httpserver.stop(do_test_finished);
}



function handler1(metadata, response)
{
  response.setStatusLine(metadata.httpVersion, 200, "OK");
  response.setHeader("Content-Disposition", "attachment; filename=foo");
  response.setHeader("Content-Type", "text/plain", false);
  var body = "foo";
}

function completeTest1(request, data, ctx)
{
  try {
    var chan = request.QueryInterface(Ci.nsIChannel);
    do_check_eq(chan.contentDisposition, chan.DISPOSITION_ATTACHMENT);
    do_check_eq(chan.contentDispositionFilename, "foo");
    do_check_eq(chan.contentDispositionHeader, "attachment; filename=foo");
  } catch (ex) {
    do_throw("error parsing Content-Disposition: " + ex);
  }
  runNextTest();  
}



function handler2(metadata, response)
{
  response.setStatusLine(metadata.httpVersion, 200, "OK");
  response.setHeader("Content-Type", "text/plain", false);
  response.setHeader("Content-Disposition", "attachment");
  var body = "foo";
  response.bodyOutputStream.write(body, body.length);
}

function completeTest2(request, data, ctx)
{
  try {
    var chan = request.QueryInterface(Ci.nsIChannel);
    do_check_eq(chan.contentDisposition, chan.DISPOSITION_ATTACHMENT);
    do_check_eq(chan.contentDispositionHeader, "attachment");

    filename = chan.contentDispositionFilename;  
    do_throw("Should have failed getting Content-Disposition filename");
  } catch (ex) {
    do_print("correctly ate exception");    
  }
  runNextTest();  
}



function handler3(metadata, response)
{
  response.setStatusLine(metadata.httpVersion, 200, "OK");
  response.setHeader("Content-Type", "text/plain", false);
  response.setHeader("Content-Disposition", "attachment; filename=");
  var body = "foo";
  response.bodyOutputStream.write(body, body.length);
}

function completeTest3(request, data, ctx)
{
  try {
    var chan = request.QueryInterface(Ci.nsIChannel);
    do_check_eq(chan.contentDisposition, chan.DISPOSITION_ATTACHMENT);
    do_check_eq(chan.contentDispositionHeader, "attachment; filename=");

    filename = chan.contentDispositionFilename;  
    do_throw("Should have failed getting Content-Disposition filename");
  } catch (ex) {
    do_print("correctly ate exception");    
  }
  runNextTest();  
}



function handler4(metadata, response)
{
  response.setStatusLine(metadata.httpVersion, 200, "OK");
  response.setHeader("Content-Type", "text/plain", false);
  response.setHeader("Content-Disposition", "inline");
  var body = "foo";
  response.bodyOutputStream.write(body, body.length);
}

function completeTest4(request, data, ctx)
{
  try {
    var chan = request.QueryInterface(Ci.nsIChannel);
    do_check_eq(chan.contentDisposition, chan.DISPOSITION_INLINE);
    do_check_eq(chan.contentDispositionHeader, "inline");

    filename = chan.contentDispositionFilename;  
    do_throw("Should have failed getting Content-Disposition filename");
  } catch (ex) {
    do_print("correctly ate exception");    
  }
  runNextTest();
}
