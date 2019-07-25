







do_load_httpd_js();

var httpserver = new nsHttpServer();
var index = 0;
var test_flags = new Array();
var testPathBase = "/dupe_hdrs";

function run_test()
{
  httpserver.start(4444);

  do_test_pending();
  run_test_number(1);
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



test_flags[1] = CL_EXPECT_FAILURE;

function handler1(metadata, response)
{
  var body = "012345678901234567890123456789";
  
  
  response.seizePower();
  response.write("HTTP/1.0 200 OK\r\n");
  response.write("Content-Type: text/plain\r\n");
  response.write("Content-Length: 30\r\n");
  response.write("Content-Length: 20\r\n");
  response.write("\r\n");
  response.write(body);
  response.finish();
}


function completeTest1(request, data, ctx)
{
  do_check_eq(request.status, Components.results.NS_ERROR_CORRUPTED_CONTENT);

  run_test_number(2);
}




function handler2(metadata, response)
{
  var body = "012345678901234567890123456789";
  response.seizePower();
  response.write("HTTP/1.0 200 OK\r\n");
  response.write("Content-Type: text/plain\r\n");
  response.write("Content-Length: 30\r\n");
  response.write("Content-Length: 30\r\n");
  response.write("\r\n");
  response.write(body);
  response.finish();
}

function completeTest2(request, data, ctx)
{
  do_check_eq(request.status, 0);
  run_test_number(3);
}



test_flags[3] = CL_EXPECT_FAILURE;

function handler3(metadata, response)
{
  var body = "012345678901234567890123456789";
  response.seizePower();
  response.write("HTTP/1.0 200 OK\r\n");
  response.write("Content-Type: text/plain\r\n");
  response.write("Content-Length: 30\r\n");
  response.write("Content-Length:\r\n");
  response.write("\r\n");
  response.write(body);
  response.finish();
}

function completeTest3(request, data, ctx)
{
  do_check_eq(request.status, Components.results.NS_ERROR_CORRUPTED_CONTENT);

  run_test_number(4);
}




test_flags[4] = CL_EXPECT_FAILURE;

function handler4(metadata, response)
{
  var body = "012345678901234567890123456789";

  response.seizePower();
  response.write("HTTP/1.0 200 OK\r\n");
  response.write("Content-Type: text/plain\r\n");
  response.write("Content-Length: 30\r\n");

  
  var evilBody = "We are the Evil bytes, Evil bytes, Evil bytes!";
  response.write("Content-Length:\r\n");
  response.write("Content-Length: %s\r\n\r\n%s" % (evilBody.length, evilBody));
  response.write("\r\n");
  response.write(body);
  response.finish();
}

function completeTest4(request, data, ctx)
{
  do_check_eq(request.status, Components.results.NS_ERROR_CORRUPTED_CONTENT);

  run_test_number(5);
}






function handler5(metadata, response)
{
  var body = "012345678901234567890123456789";
  response.seizePower();
  response.write("HTTP/1.0 200 OK\r\n");
  response.write("Content-Type: text/plain\r\n");
  response.write("Content-Length: 30\r\n");
  response.write("Referer: naive.org\r\n");
  response.write("Referer: evil.net\r\n");
  response.write("\r\n");
  response.write(body);
  response.finish();
}

function completeTest5(request, data, ctx)
{
  try {
    referer = request.getResponseHeader("Referer");
    do_check_eq(referer, "naive.org");
  } catch (ex) {
    do_throw("Referer header should be present");
  }

  run_test_number(6);
}




test_flags[6] = CL_EXPECT_FAILURE;

function handler6(metadata, response)
{
  var body = "012345678901234567890123456789";
  response.seizePower();
  response.write("HTTP/1.0 301 Moved\r\n");
  response.write("Content-Type: text/plain\r\n");
  response.write("Content-Length: 30\r\n");
  response.write("Location: http://localhost:4444/content\r\n");
  response.write("Location: http://www.microsoft.com/\r\n");
  response.write("Connection: close/\r\n");
  response.write("\r\n");
  response.write(body);
  response.finish();
}

function completeTest6(request, data, ctx)
{
  do_check_eq(request.status, Components.results.NS_ERROR_CORRUPTED_CONTENT);


  run_test_number(8);
}




function handler7(metadata, response)
{
  var body = "012345678901234567890123456789";
  response.seizePower();
  response.write("HTTP/1.0 301 Moved\r\n");
  response.write("Content-Type: text/plain\r\n");
  response.write("Content-Length: 30\r\n");
  
  response.write("Location: http://localhost:4444" + testPathBase + "5\r\n");
  response.write("Location: http://localhost:4444" + testPathBase + "5\r\n");
  response.write("Connection: close/\r\n");
  response.write("\r\n");
  response.write(body);
  response.finish();
}

function completeTest7(request, data, ctx)
{
  
  request.QueryInterface(Components.interfaces.nsIHttpChannel);

  try {
    referer = request.getResponseHeader("Referer");
    do_check_eq(referer, "naive.org");
  } catch (ex) {
    do_throw("Referer header should be present");
  }

  run_test_number(8);
}



test_flags[8] = CL_EXPECT_FAILURE;

function handler8(metadata, response)
{
  var body = "012345678901234567890123456789";
  response.seizePower();
  response.write("HTTP/1.0 301 Moved\r\n");
  response.write("Content-Type: text/plain\r\n");
  response.write("Content-Length: 30\r\n");
  
  response.write("Location: http://localhost:4444" + testPathBase + "4\r\n");
  response.write("Location:\r\n");
  response.write("Connection: close/\r\n");
  response.write("\r\n");
  response.write(body);
  response.finish();
}

function completeTest8(request, data, ctx)
{
  do_check_eq(request.status, Components.results.NS_ERROR_CORRUPTED_CONTENT);

  run_test_number(9);
}




test_flags[9] = CL_EXPECT_FAILURE;

function handler9(metadata, response)
{
  var body = "012345678901234567890123456789";
  response.seizePower();
  response.write("HTTP/1.0 301 Moved\r\n");
  response.write("Content-Type: text/plain\r\n");
  response.write("Content-Length: 30\r\n");
  
  response.write("Location: http://localhost:4444" + testPathBase + "2\r\n");
  response.write("Location:\r\n");
  
  response.write("Location: http://localhost:4444" + testPathBase + "4\r\n");
  response.write("Connection: close/\r\n");
  response.write("\r\n");
  response.write(body);
  response.finish();
}

function completeTest9(request, data, ctx)
{
  
  do_check_eq(request.status, Components.results.NS_ERROR_CORRUPTED_CONTENT);

  run_test_number(10);
}



test_flags[10] = CL_EXPECT_FAILURE;

function handler10(metadata, response)
{
  var body = "012345678901234567890123456789";
  response.seizePower();
  response.write("HTTP/1.0 200 OK\r\n");
  response.write("Content-Type: text/plain\r\n");
  response.write("Content-Length: 30\r\n");
  response.write("Content-Disposition: attachment; filename=foo\r\n");
  response.write("Content-Disposition: attachment; filename=bar\r\n");
  response.write("Content-Disposition: attachment; filename=baz\r\n");
  response.write("\r\n");
  response.write(body);
  response.finish();
}


function completeTest10(request, data, ctx)
{
  do_check_eq(request.status, Components.results.NS_ERROR_CORRUPTED_CONTENT);

  run_test_number(11);
}




function handler11(metadata, response)
{
  var body = "012345678901234567890123456789";
  response.seizePower();
  response.write("HTTP/1.0 200 OK\r\n");
  response.write("Content-Type: text/plain\r\n");
  response.write("Content-Length: 30\r\n");
  response.write("Content-Disposition: attachment; filename=foo\r\n");
  response.write("Content-Disposition: attachment; filename=foo\r\n");
  response.write("\r\n");
  response.write(body);
  response.finish();
}

function completeTest11(request, data, ctx)
{
  do_check_eq(request.status, 0);

  try {
    var chan = request.QueryInterface(Ci.nsIChannel);
    do_check_eq(chan.contentDisposition, chan.DISPOSITION_ATTACHMENT);
    do_check_eq(chan.contentDispositionFilename, "foo");
    do_check_eq(chan.contentDispositionHeader, "attachment; filename=foo");
  } catch (ex) {
    do_throw("error parsing Content-Disposition: " + ex);
  }

  run_test_number(12);
}



test_flags[12] = CL_EXPECT_FAILURE;

function handler12(metadata, response)
{
  var body = "012345678901234567890123456789";
  response.seizePower();
  response.write("HTTP/1.0 301 Moved\r\n");
  response.write("Content-Type: text/plain\r\n");
  response.write("Content-Length: 30\r\n");
  response.write("Location:\r\n");
  response.write("Connection: close/\r\n");
  response.write("\r\n");
  response.write(body);
  response.finish();
}

function completeTest12(request, data, ctx)
{
  do_check_eq(request.status, Components.results.NS_ERROR_CORRUPTED_CONTENT);

  endTests();
}

