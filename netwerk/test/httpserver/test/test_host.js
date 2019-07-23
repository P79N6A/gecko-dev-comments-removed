










































const PORT = 4444;
const FAKE_PORT_ONE = 8888;
const FAKE_PORT_TWO = 8889;

var srv, id;

function run_test()
{
  dumpn("*** run_test");

  srv = createServer();

  srv.registerPathHandler("/http/1.0-request", http10Request);
  srv.registerPathHandler("/http/1.1-good-host", http11goodHost);
  srv.registerPathHandler("/http/1.1-good-host-wacky-port",
                          http11goodHostWackyPort);
  srv.registerPathHandler("/http/1.1-ip-host", http11ipHost);

  srv.start(FAKE_PORT_ONE);

  id = srv.identity;

  
  
  
  do_check_eq(id.primaryScheme, "http");
  do_check_eq(id.primaryHost, "localhost");
  do_check_eq(id.primaryPort, FAKE_PORT_ONE);
  do_check_true(id.has("http", "localhost", FAKE_PORT_ONE));
  do_check_true(id.has("http", "127.0.0.1", FAKE_PORT_ONE));

  
  id.add("http", "localhost", FAKE_PORT_ONE);
  do_check_eq(id.primaryScheme, "http");
  do_check_eq(id.primaryHost, "localhost");
  do_check_eq(id.primaryPort, FAKE_PORT_ONE);
  do_check_true(id.has("http", "localhost", FAKE_PORT_ONE));
  do_check_true(id.has("http", "127.0.0.1", FAKE_PORT_ONE));

  
  id.setPrimary("http", "127.0.0.1", FAKE_PORT_ONE);
  do_check_eq(id.primaryScheme, "http");
  do_check_eq(id.primaryHost, "127.0.0.1");
  do_check_eq(id.primaryPort, FAKE_PORT_ONE);
  do_check_true(id.has("http", "localhost", FAKE_PORT_ONE));
  do_check_true(id.has("http", "127.0.0.1", FAKE_PORT_ONE));

  
  
  id.remove("http", "127.0.0.1", FAKE_PORT_ONE);
  do_check_eq(id.primaryScheme, "http");
  do_check_eq(id.primaryHost, "localhost");
  do_check_eq(id.primaryPort, FAKE_PORT_ONE);
  do_check_true(id.has("http", "localhost", FAKE_PORT_ONE));
  do_check_false(id.has("http", "127.0.0.1", FAKE_PORT_ONE));

  
  
  id.remove("http", "localhost", FAKE_PORT_ONE);
  do_check_eq(id.primaryScheme, "http");
  do_check_eq(id.primaryHost, "localhost");
  do_check_eq(id.primaryPort, FAKE_PORT_ONE);
  do_check_true(id.has("http", "localhost", FAKE_PORT_ONE));
  do_check_false(id.has("http", "127.0.0.1", FAKE_PORT_ONE));

  
  
  
  do_test_pending();
  srv.stop(function()
  {
    try
    {
      do_test_pending();
      run_test_2();
    }
    finally
    {
      do_test_finished();
    }
  });
}

function run_test_2()
{
  dumpn("*** run_test_2");

  do_test_finished();

  
  
  checkPrimariesThrow(id);
  do_check_false(id.has("http", "127.0.0.1", FAKE_PORT_ONE));
  do_check_false(id.has("http", "localhost", FAKE_PORT_ONE));

  srv.start(FAKE_PORT_TWO);

  
  
  do_check_eq(id.primaryScheme, "http");
  do_check_eq(id.primaryHost, "localhost", FAKE_PORT_TWO);
  do_check_eq(id.primaryPort, FAKE_PORT_TWO);
  do_check_false(id.has("http", "localhost", FAKE_PORT_ONE));
  do_check_false(id.has("http", "127.0.0.1", FAKE_PORT_ONE));
  do_check_true(id.has("http", "localhost", FAKE_PORT_TWO));
  do_check_true(id.has("http", "127.0.0.1", FAKE_PORT_TWO));

  
  
  
  id.setPrimary("http", "example.com", FAKE_PORT_TWO);
  do_check_true(id.has("http", "example.com", FAKE_PORT_TWO));
  do_check_true(id.has("http", "127.0.0.1", FAKE_PORT_TWO));
  do_check_true(id.has("http", "localhost", FAKE_PORT_TWO));
  do_check_false(id.has("http", "127.0.0.1", FAKE_PORT_ONE));
  do_check_false(id.has("http", "localhost", FAKE_PORT_ONE));

  id.remove("http", "localhost", FAKE_PORT_TWO);
  do_check_true(id.has("http", "example.com", FAKE_PORT_TWO));
  do_check_false(id.has("http", "localhost", FAKE_PORT_TWO));
  do_check_true(id.has("http", "127.0.0.1", FAKE_PORT_TWO));
  do_check_false(id.has("http", "localhost", FAKE_PORT_ONE));
  do_check_false(id.has("http", "127.0.0.1", FAKE_PORT_ONE));

  id.remove("http", "127.0.0.1", FAKE_PORT_TWO);
  do_check_true(id.has("http", "example.com", FAKE_PORT_TWO));
  do_check_false(id.has("http", "localhost", FAKE_PORT_TWO));
  do_check_false(id.has("http", "127.0.0.1", FAKE_PORT_TWO));
  do_check_false(id.has("http", "localhost", FAKE_PORT_ONE));
  do_check_false(id.has("http", "127.0.0.1", FAKE_PORT_ONE));

  do_test_pending();
  srv.stop(function()
  {
    try
    {
      do_test_pending();
      run_test_3();
    }
    finally
    {
      do_test_finished();
    }
  });
}

function run_test_3()
{
  dumpn("*** run_test_3");

  do_test_finished();

  
  
  
  do_check_eq(id.primaryScheme, "http");
  do_check_eq(id.primaryHost, "example.com");
  do_check_eq(id.primaryPort, FAKE_PORT_TWO);
  do_check_true(id.has("http", "example.com", FAKE_PORT_TWO));
  do_check_false(id.has("http", "localhost", FAKE_PORT_TWO));
  do_check_false(id.has("http", "127.0.0.1", FAKE_PORT_TWO));
  do_check_false(id.has("http", "localhost", FAKE_PORT_ONE));
  do_check_false(id.has("http", "127.0.0.1", FAKE_PORT_ONE));

  srv.start(PORT);

  
  do_check_true(id.has("http", "example.com", FAKE_PORT_TWO));
  do_check_false(id.has("http", "localhost", FAKE_PORT_TWO));
  do_check_false(id.has("http", "127.0.0.1", FAKE_PORT_TWO));
  do_check_false(id.has("http", "localhost", FAKE_PORT_ONE));
  do_check_false(id.has("http", "127.0.0.1", FAKE_PORT_ONE));
  do_check_true(id.has("http", "localhost", PORT));
  do_check_true(id.has("http", "127.0.0.1", PORT));

  
  id.remove("http", "example.com", FAKE_PORT_TWO);

  
  
  id.add("http", "localhost", 80);

  
  
  
  do_check_true(id.has("http", "localhost", 80));
  do_check_eq(id.primaryScheme, "http");
  do_check_eq(id.primaryHost, "localhost");
  do_check_eq(id.primaryPort, PORT);
  do_check_true(id.has("http", "localhost", PORT));
  do_check_true(id.has("http", "127.0.0.1", PORT));
  do_check_false(id.has("http", "localhost", FAKE_PORT_ONE));
  do_check_false(id.has("http", "127.0.0.1", FAKE_PORT_ONE));
  do_check_false(id.has("http", "example.com", FAKE_PORT_TWO));
  do_check_false(id.has("http", "localhost", FAKE_PORT_TWO));
  do_check_false(id.has("http", "127.0.0.1", FAKE_PORT_TWO));

  
  
  runRawTests(tests, testComplete(srv));
}













function checkPrimariesThrow(id)
{
  var threw = false;
  try
  {
    id.primaryScheme;
  }
  catch (e)
  {
    threw = e === Cr.NS_ERROR_NOT_INITIALIZED;
  }
  do_check_true(threw);

  threw = false;
  try
  {
    id.primaryHost;
  }
  catch (e)
  {
    threw = e === Cr.NS_ERROR_NOT_INITIALIZED;
  }
  do_check_true(threw);

  threw = false;
  try
  {
    id.primaryPort;
  }
  catch (e)
  {
    threw = e === Cr.NS_ERROR_NOT_INITIALIZED;
  }
  do_check_true(threw);
}




function check400(data)
{
  var iter = LineIterator(data);

  
  var firstLine = iter.next();
  do_check_eq(firstLine.substring(0, HTTP_400_LEADER_LENGTH), HTTP_400_LEADER);
}






const HTTP_400_LEADER = "HTTP/1.1 400 ";
const HTTP_400_LEADER_LENGTH = HTTP_400_LEADER.length;

var test, data;
var tests = [];



function http10Request(request, response)
{
  writeDetails(request, response);
  response.setStatusLine("1.0", 200, "TEST PASSED");
}
data = "GET /http/1.0-request HTTP/1.0\r\n" +
       "\r\n";
function check10(data)
{
  var iter = LineIterator(data);

  
  do_check_eq(iter.next(), "HTTP/1.0 200 TEST PASSED");

  skipHeaders(iter);

  
  var body =
    [
     "Method:  GET",
     "Path:    /http/1.0-request",
     "Query:   ",
     "Version: 1.0",
     "Scheme:  http",
     "Host:    localhost",
     "Port:    4444",
    ];

  expectLines(iter, body);
}
test = new RawTest("localhost", PORT, data, check10),
tests.push(test);




data = "GET /http/1.1-request HTTP/1.1\r\n" +
       "\r\n";
test = new RawTest("localhost", PORT, data, check400),
tests.push(test);




data = "GET /http/1.1-request HTTP/1.1\r\n" +
       "Host: not-localhost\r\n" +
       "\r\n";
test = new RawTest("localhost", PORT, data, check400),
tests.push(test);




data = "GET /http/1.1-request HTTP/1.1\r\n" +
       "Host: not-localhost:4444\r\n" +
       "\r\n";
test = new RawTest("localhost", PORT, data, check400),
tests.push(test);




data = "GET /http/1.1-request HTTP/1.1\r\n" +
       "Host: 127.0.0.1\r\n" +
       "\r\n";
test = new RawTest("localhost", PORT, data, check400),
tests.push(test);




data = "GET http://127.0.0.1/http/1.1-request HTTP/1.1\r\n" +
       "Host: 127.0.0.1\r\n" +
       "\r\n";
test = new RawTest("localhost", PORT, data, check400),
tests.push(test);




data = "GET http://localhost:31337/http/1.1-request HTTP/1.1\r\n" +
       "Host: localhost:31337\r\n" +
       "\r\n";
test = new RawTest("localhost", PORT, data, check400),
tests.push(test);




data = "GET https://localhost:4444/http/1.1-request HTTP/1.1\r\n" +
       "Host: localhost:4444\r\n" +
       "\r\n";
test = new RawTest("localhost", PORT, data, check400),
tests.push(test);




function http11goodHost(request, response)
{
  writeDetails(request, response);
  response.setStatusLine("1.1", 200, "TEST PASSED");
}
data = "GET /http/1.1-good-host HTTP/1.1\r\n" +
       "Host: localhost:4444\r\n" +
       "\r\n";
function check11goodHost(data)
{
  var iter = LineIterator(data);

  
  do_check_eq(iter.next(), "HTTP/1.1 200 TEST PASSED");

  skipHeaders(iter);

  
  var body =
    [
     "Method:  GET",
     "Path:    /http/1.1-good-host",
     "Query:   ",
     "Version: 1.1",
     "Scheme:  http",
     "Host:    localhost",
     "Port:    4444",
    ];

  expectLines(iter, body);
}
test = new RawTest("localhost", PORT, data, check11goodHost),
tests.push(test);




function http11ipHost(request, response)
{
  writeDetails(request, response);
  response.setStatusLine("1.1", 200, "TEST PASSED");
}
data = "GET /http/1.1-ip-host HTTP/1.1\r\n" +
       "Host: 127.0.0.1:4444\r\n" +
       "\r\n";
function check11ipHost(data)
{
  var iter = LineIterator(data);

  
  do_check_eq(iter.next(), "HTTP/1.1 200 TEST PASSED");

  skipHeaders(iter);

  
  var body =
    [
     "Method:  GET",
     "Path:    /http/1.1-ip-host",
     "Query:   ",
     "Version: 1.1",
     "Scheme:  http",
     "Host:    127.0.0.1",
     "Port:    4444",
    ];

  expectLines(iter, body);
}
test = new RawTest("localhost", PORT, data, check11ipHost),
tests.push(test);






data = "GET http://localhost:4444/http/1.1-good-host HTTP/1.1\r\n" +
       "Host: localhost:4444\r\n" +
       "\r\n";
test = new RawTest("localhost", PORT, data, check11goodHost),
tests.push(test);






data = "GET http://localhost:4444/http/1.1-good-host HTTP/1.1\r\n" +
       "Host: localhost:1234\r\n" +
       "\r\n";
test = new RawTest("localhost", PORT, data, check11goodHost),
tests.push(test);






data = "GET http://localhost:4444/http/1.1-good-host HTTP/1.1\r\n" +
       "Host: not-localhost:4444\r\n" +
       "\r\n";
test = new RawTest("localhost", PORT, data, check11goodHost),
tests.push(test);






data = "GET http://localhost:4444/http/1.1-good-host HTTP/1.1\r\n" +
       "Host: yippity-skippity\r\n" +
       "\r\n";
function checkInaccurate(data)
{
  check11goodHost(data);

  
  srv.identity.setPrimary("http", "127.0.0.1", 4444);
}
test = new RawTest("localhost", PORT, data, checkInaccurate),
tests.push(test);






data = "GET /http/1.0-request HTTP/1.0\r\n" +
       "Host: not-localhost:4444\r\n" +
       "\r\n";
function check10ip(data)
{
  var iter = LineIterator(data);

  
  do_check_eq(iter.next(), "HTTP/1.0 200 TEST PASSED");

  skipHeaders(iter);

  
  var body =
    [
     "Method:  GET",
     "Path:    /http/1.0-request",
     "Query:   ",
     "Version: 1.0",
     "Scheme:  http",
     "Host:    127.0.0.1",
     "Port:    4444",
    ];

  expectLines(iter, body);
}
test = new RawTest("localhost", PORT, data, check10ip),
tests.push(test);




function http11goodHostWackyPort(request, response)
{
  writeDetails(request, response);
  response.setStatusLine("1.1", 200, "TEST PASSED");
}
data = "GET /http/1.1-good-host-wacky-port HTTP/1.1\r\n" +
       "Host: localhost\r\n" +
       "\r\n";
function check11goodHostWackyPort(data)
{
  var iter = LineIterator(data);

  
  do_check_eq(iter.next(), "HTTP/1.1 200 TEST PASSED");

  skipHeaders(iter);

  
  var body =
    [
     "Method:  GET",
     "Path:    /http/1.1-good-host-wacky-port",
     "Query:   ",
     "Version: 1.1",
     "Scheme:  http",
     "Host:    localhost",
     "Port:    80",
    ];

  expectLines(iter, body);
}
test = new RawTest("localhost", PORT, data, check11goodHostWackyPort),
tests.push(test);




data = "GET /http/1.1-good-host-wacky-port HTTP/1.1\r\n" +
       "Host: localhost:\r\n" +
       "\r\n";
test = new RawTest("localhost", PORT, data, check11goodHostWackyPort),
tests.push(test);




data = "GET http://localhost/http/1.1-good-host-wacky-port HTTP/1.1\r\n" +
       "Host: localhost\r\n" +
       "\r\n";
test = new RawTest("localhost", PORT, data, check11goodHostWackyPort),
tests.push(test);




data = "GET http://localhost:/http/1.1-good-host-wacky-port HTTP/1.1\r\n" +
       "Host: localhost\r\n" +
       "\r\n";
test = new RawTest("localhost", PORT, data, check11goodHostWackyPort),
tests.push(test);




data = "GET http://localhost:80/http/1.1-good-host-wacky-port HTTP/1.1\r\n" +
       "Host: who-cares\r\n" +
       "\r\n";
test = new RawTest("localhost", PORT, data, check11goodHostWackyPort),
tests.push(test);




data = "GET is-this-the-real-life-is-this-just-fantasy HTTP/1.1\r\n" +
       "Host: localhost:4444\r\n" +
       "\r\n";
test = new RawTest("localhost", PORT, data, check400),
tests.push(test);




data = "GET /http/1.1-request HTTP/1.1\r\n" +
       "Host: la la la\r\n" +
       "\r\n";
test = new RawTest("localhost", PORT, data, check400),
tests.push(test);




data = "GET http://localhost:4444/http/1.1-good-host HTTP/1.1\r\n" +
       "Host: la la la\r\n" +
       "\r\n";
test = new RawTest("localhost", PORT, data, check11goodHost),
tests.push(test);




data = "GET http://localhost:4444/http/1.1-request HTTP/1.0\r\n" +
       "Host: localhost:4444\r\n" +
       "\r\n";
test = new RawTest("localhost", PORT, data, check400),
tests.push(test);




data = "GET http://not-localhost:4444/http/1.1-request HTTP/1.1\r\n" +
       "Host: not-localhost:4444\r\n" +
       "\r\n";
test = new RawTest("localhost", PORT, data, check400),
tests.push(test);




data = "GET http://not-localhost:4444/http/1.1-request HTTP/1.1\r\n" +
       "Host: localhost:4444\r\n" +
       "\r\n";
test = new RawTest("localhost", PORT, data, check400),
tests.push(test);
