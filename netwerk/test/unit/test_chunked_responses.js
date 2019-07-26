






Cu.import("resource://testing-common/httpd.js");

XPCOMUtils.defineLazyGetter(this, "URL", function() {
  return "http://localhost:" + httpserver.identity.primaryPort;
});

var httpserver = new HttpServer();
var index = 0;
var test_flags = new Array();
var testPathBase = "/chunked_hdrs";

function run_test()
{
  httpserver.start(-1);

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
  var chan = ios.newChannel(URL + url, "", null);
  var httpChan = chan.QueryInterface(Components.interfaces.nsIHttpChannel);
  return httpChan;
}

function endTests()
{
  httpserver.stop(do_test_finished);
}




test_flags[1] = CL_EXPECT_LATE_FAILURE|CL_ALLOW_UNKNOWN_CL;

function handler1(metadata, response)
{
  var body = "12345678123456789\r\ndata never reached";

  response.seizePower();
  response.write("HTTP/1.1 200 OK\r\n");
  response.write("Content-Type: text/plain\r\n");
  response.write("Transfer-Encoding: chunked\r\n");
  response.write("\r\n");
  response.write(body);
  response.finish();
}

function completeTest1(request, data, ctx)
{
  do_check_eq(request.status, Components.results.NS_ERROR_UNEXPECTED);

  run_test_number(2);
}




test_flags[2] = CL_EXPECT_LATE_FAILURE|CL_ALLOW_UNKNOWN_CL;

function handler2(metadata, response)
{
  var body = "junkintheway 123\r\ndata never reached";

  response.seizePower();
  response.write("HTTP/1.1 200 OK\r\n");
  response.write("Content-Type: text/plain\r\n");
  response.write("Transfer-Encoding: chunked\r\n");
  response.write("\r\n");
  response.write(body);
  response.finish();
}

function completeTest2(request, data, ctx)
{
  do_check_eq(request.status, Components.results.NS_ERROR_UNEXPECTED);
  run_test_number(3);
}




test_flags[3] = CL_ALLOW_UNKNOWN_CL;

function handler3(metadata, response)
{
  var body = "c junkafter\r\ndata reached\r\n0\r\n\r\n";

  response.seizePower();
  response.write("HTTP/1.1 200 OK\r\n");
  response.write("Content-Type: text/plain\r\n");
  response.write("Transfer-Encoding: chunked\r\n");
  response.write("\r\n");
  response.write(body);
  response.finish();
}

function completeTest3(request, data, ctx)
{
  do_check_eq(request.status, 0);
  run_test_number(4);
}




test_flags[4] = CL_ALLOW_UNKNOWN_CL;

function handler4(metadata, response)
{
  var body = "c\r\ndata reached\r\n3\r\nhej\r\n0\r\n\r\n";

  response.seizePower();
  response.write("HTTP/1.1 200 OK\r\n");
  response.write("Content-Type: text/plain\r\n");
  response.write("Transfer-Encoding: chunked\r\n");
  response.write("\r\n");
  response.write(body);
  response.finish();
}

function completeTest4(request, data, ctx)
{
  do_check_eq(request.status, 0);
  run_test_number(5);
}





test_flags[5] = CL_EXPECT_LATE_FAILURE|CL_ALLOW_UNKNOWN_CL;

function handler5(metadata, response)
{
  var body = "123456781\r\ndata never reached";

  response.seizePower();
  response.write("HTTP/1.1 200 OK\r\n");
  response.write("Content-Type: text/plain\r\n");
  response.write("Transfer-Encoding: chunked\r\n");
  response.write("\r\n");
  response.write(body);
  response.finish();
}

function completeTest5(request, data, ctx)
{
  do_check_eq(request.status, Components.results.NS_ERROR_UNEXPECTED);
  endTests();

}
