











var srv = createServer();
srv.start(-1);
const PORT = srv.identity.primaryPort;

function run_test()
{
  srv.registerPathHandler("/lots-of-leading-blank-lines",
                          lotsOfLeadingBlankLines);
  srv.registerPathHandler("/very-long-request-line",
                          veryLongRequestLine);

  runRawTests(tests, testComplete(srv));
}






var test, data, str;
var tests = [];


function veryLongRequestLine(request, response)
{
  writeDetails(request, response);
  response.setStatusLine(request.httpVersion, 200, "TEST PASSED");
}

var path = "/very-long-request-line?";
var reallyLong = "0123456789ABCDEF0123456789ABCDEF"; 
reallyLong = reallyLong + reallyLong + reallyLong + reallyLong; 
reallyLong = reallyLong + reallyLong + reallyLong + reallyLong; 
reallyLong = reallyLong + reallyLong + reallyLong + reallyLong; 
reallyLong = reallyLong + reallyLong + reallyLong + reallyLong; 
reallyLong = reallyLong + reallyLong + reallyLong + reallyLong; 
reallyLong = reallyLong + reallyLong + reallyLong + reallyLong; 
reallyLong = reallyLong + reallyLong + reallyLong + reallyLong; 
if (reallyLong.length !== 524288)
  throw new TypeError("generated length not as long as expected");
str = "GET /very-long-request-line?" + reallyLong + " HTTP/1.1\r\n" +
      "Host: localhost:" + PORT + "\r\n" +
      "\r\n";
data = [];
for (var i = 0; i < str.length; i += 16384)
  data.push(str.substr(i, 16384));

function checkVeryLongRequestLine(data)
{
  var iter = LineIterator(data);

  print("data length: " + data.length);
  print("iter object: " + iter);

  
  do_check_eq(iter.next(), "HTTP/1.1 200 TEST PASSED");

  skipHeaders(iter);

  
  var body =
    [
     "Method:  GET",
     "Path:    /very-long-request-line",
     "Query:   " + reallyLong,
     "Version: 1.1",
     "Scheme:  http",
     "Host:    localhost",
     "Port:    " + PORT,
    ];

  expectLines(iter, body);
}
test = new RawTest("localhost", PORT, data, checkVeryLongRequestLine),
tests.push(test);


function lotsOfLeadingBlankLines(request, response)
{
  writeDetails(request, response);
  response.setStatusLine(request.httpVersion, 200, "TEST PASSED");
}

var blankLines = "\r\n";
for (var i = 0; i < 14; i++)
  blankLines += blankLines;
str = blankLines +
      "GET /lots-of-leading-blank-lines HTTP/1.1\r\n" +
      "Host: localhost:" + PORT + "\r\n" +
      "\r\n";
data = [];
for (var i = 0; i < str.length; i += 100)
  data.push(str.substr(i, 100));

function checkLotsOfLeadingBlankLines(data)
{
  var iter = LineIterator(data);

  
  print("data length: " + data.length);
  print("iter object: " + iter);

  do_check_eq(iter.next(), "HTTP/1.1 200 TEST PASSED");

  skipHeaders(iter);

  
  var body =
    [
     "Method:  GET",
     "Path:    /lots-of-leading-blank-lines",
     "Query:   ",
     "Version: 1.1",
     "Scheme:  http",
     "Host:    localhost",
     "Port:    " + PORT,
    ];

  expectLines(iter, body);
}

test = new RawTest("localhost", PORT, data, checkLotsOfLeadingBlankLines),
tests.push(test);
