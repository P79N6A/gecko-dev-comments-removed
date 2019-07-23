










































const PORT = 4444;

var srv;

function run_test()
{
  srv = createServer();

  srv.registerPathHandler("/raw-data", handleRawData);
  srv.registerPathHandler("/called-too-late", handleTooLate);
  srv.registerPathHandler("/exceptions", handleExceptions);
  srv.registerPathHandler("/async-seizure", handleAsyncSeizure);
  srv.registerPathHandler("/seize-after-async", handleSeizeAfterAsync);
  srv.registerPathHandler("/thrown-exception", handleThrownException);
  srv.registerPathHandler("/asap-later-write", handleASAPLaterWrite);
  srv.registerPathHandler("/asap-later-finish", handleASAPLaterFinish);

  srv.start(PORT);

  runRawTests(tests, testComplete(srv));
}


function checkException(fun, err, msg)
{
  try
  {
    fun();
  }
  catch (e)
  {
    if (e !== err && e.result !== err)
      do_throw(msg);
    return;
  }
  do_throw(msg);
}

function callASAPLater(fun)
{
  gThreadManager.currentThread.dispatch({
    run: function()
    {
      fun();
    }
  }, Ci.nsIThread.DISPATCH_NORMAL);
}






function handleRawData(request, response)
{
  response.seizePower();
  response.write("Raw data!");
  response.finish();
}

function handleTooLate(request, response)
{
  response.write("DO NOT WANT");
  var output = response.bodyOutputStream;

  response.seizePower();

  if (response.bodyOutputStream !== output)
    response.write("bodyOutputStream changed!");
  else
    response.write("too-late passed");
  response.finish();
}

function handleExceptions(request, response)
{
  response.seizePower();
  checkException(function() { response.setStatusLine("1.0", 500, "ISE"); },
                 Cr.NS_ERROR_NOT_AVAILABLE,
                 "setStatusLine should throw not-available after seizePower");
  checkException(function() { response.setHeader("X-Fail", "FAIL", false); },
                 Cr.NS_ERROR_NOT_AVAILABLE,
                 "setHeader should throw not-available after seizePower");
  checkException(function() { response.processAsync(); },
                 Cr.NS_ERROR_NOT_AVAILABLE,
                 "processAsync should throw not-available after seizePower");
  var out = response.bodyOutputStream;
  var data = "exceptions test passed";
  out.write(data, data.length);
  response.seizePower(); 
  response.finish();
  response.finish(); 
  checkException(function() { response.seizePower(); },
                 Cr.NS_ERROR_UNEXPECTED,
                 "seizePower should throw unexpected after finish");
}

function handleAsyncSeizure(request, response)
{
  response.seizePower();
  callLater(1, function()
  {
    response.write("async seizure passed");
    response.bodyOutputStream.close();
    callLater(1, function()
    {
      response.finish();
    });
  });
}

function handleSeizeAfterAsync(request, response)
{
  response.setStatusLine(request.httpVersion, 200, "async seizure pass");
  response.processAsync();
  checkException(function() { response.seizePower(); },
                 Cr.NS_ERROR_NOT_AVAILABLE,
                 "seizePower should throw not-available after processAsync");
  callLater(1, function()
  {
    response.finish();
  });
}

function handleThrownException(request, response)
{
  if (request.queryString === "writeBefore")
    response.write("ignore this");
  else if (request.queryString === "writeBeforeEmpty")
    response.write("");
  else if (request.queryString !== "")
    throw "query string FAIL";
  response.seizePower();
  response.write("preparing to throw...");
  throw "badness 10000";
}

function handleASAPLaterWrite(request, response)
{
  response.seizePower();
  response.write("should only ");
  response.write("see this");

  callASAPLater(function()
  {
    response.write("...and not this");
    callASAPLater(function()
    {
      response.write("...or this");
      response.finish();
    });
  });

  throw "opening pitch of the ballgame";
}

function handleASAPLaterFinish(request, response)
{
  response.seizePower();
  response.write("should only see this");

  callASAPLater(function()
  {
    response.finish();
  });

  throw "out the bum!";
}






var test, data;
var tests = [];

data = "GET /raw-data HTTP/1.0\r\n" +
       "\r\n";
function checkRawData(data)
{
  do_check_eq(data, "Raw data!");
}
test = new RawTest("localhost", PORT, data, checkRawData),
tests.push(test);

data = "GET /called-too-late HTTP/1.0\r\n" +
       "\r\n";
function checkTooLate(data)
{
  do_check_eq(LineIterator(data).next(), "too-late passed");
}
test = new RawTest("localhost", PORT, data, checkTooLate),
tests.push(test);

data = "GET /exceptions HTTP/1.0\r\n" +
       "\r\n";
function checkExceptions(data)
{
  do_check_eq("exceptions test passed", data);
}
test = new RawTest("localhost", PORT, data, checkExceptions),
tests.push(test);

data = "GET /async-seizure HTTP/1.0\r\n" +
       "\r\n";
function checkAsyncSeizure(data)
{
  do_check_eq(data, "async seizure passed");
}
test = new RawTest("localhost", PORT, data, checkAsyncSeizure),
tests.push(test);

data = "GET /seize-after-async HTTP/1.0\r\n" +
       "\r\n";
function checkSeizeAfterAsync(data)
{
  do_check_eq(LineIterator(data).next(), "HTTP/1.0 200 async seizure pass");
}
test = new RawTest("localhost", PORT, data, checkSeizeAfterAsync),
tests.push(test);

data = "GET /thrown-exception?writeBefore HTTP/1.0\r\n" +
       "\r\n";
function checkThrownExceptionWriteBefore(data)
{
  do_check_eq(data, "preparing to throw...");
}
test = new RawTest("localhost", PORT, data, checkThrownExceptionWriteBefore),
tests.push(test);

data = "GET /thrown-exception?writeBeforeEmpty HTTP/1.0\r\n" +
       "\r\n";
function checkThrownExceptionWriteBeforeEmpty(data)
{
  do_check_eq(data, "preparing to throw...");
}
test = new RawTest("localhost", PORT, data, checkThrownExceptionWriteBeforeEmpty),
tests.push(test);

data = "GET /thrown-exception HTTP/1.0\r\n" +
       "\r\n";
function checkThrownException(data)
{
  do_check_eq(data, "preparing to throw...");
}
test = new RawTest("localhost", PORT, data, checkThrownException),
tests.push(test);

data = "GET /asap-later-write HTTP/1.0\r\n" +
       "\r\n";
function checkASAPLaterWrite(data)
{
  do_check_eq(data, "should only see this");
}
test = new RawTest("localhost", PORT, data, checkASAPLaterWrite),
tests.push(test);

data = "GET /asap-later-finish HTTP/1.0\r\n" +
       "\r\n";
function checkASAPLaterFinish(data)
{
  do_check_eq(data, "should only see this");
}
test = new RawTest("localhost", PORT, data, checkASAPLaterFinish),
tests.push(test);
