












































const PORT = 4444;

var srv;

function run_test()
{
  srv = createServer();
  srv.registerPathHandler("/lots-of-leading-blank-lines",
                          lotsOfLeadingBlankLines);
  srv.registerPathHandler("/very-long-request-line",
                          veryLongRequestLine);
  srv.start(PORT);

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
var gibberish = "dfsasdbfjkbnsldkjnewiunfasjkn";
for (var i = 0; i < 10; i++)
  gibberish += gibberish;
str = "GET /very-long-request-line?" + gibberish + " HTTP/1.1\r\n" +
      "Host: localhost:4444\r\n" +
      "\r\n";
data = [];
for (var i = 0; i < str.length; i += 50)
  data.push(str.substr(i, 50));
function checkVeryLongRequestLine(data)
{
  var iter = LineIterator(data);

  
  do_check_eq(iter.next(), "HTTP/1.1 200 TEST PASSED");

  skipHeaders(iter);

  
  var body =
    [
     "Method:  GET",
     "Path:    /very-long-request-line",
     "Query:   " + gibberish,
     "Version: 1.1",
     "Scheme:  http",
     "Host:    localhost",
     "Port:    4444",
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
      "Host: localhost:4444\r\n" +
      "\r\n";
data = [];
for (var i = 0; i < str.length; i += 100)
  data.push(str.substr(i, 100));
function checkLotsOfLeadingBlankLines(data)
{
  var iter = LineIterator(data);

  
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
     "Port:    4444",
    ];

  expectLines(iter, body);
}
test = new RawTest("localhost", PORT, data, checkLotsOfLeadingBlankLines),
tests.push(test);
