









XPCOMUtils.defineLazyGetter(this, "PORT", function() {
  return srv.identity.primaryPort;
});

var srv;

function run_test()
{
  srv = createServer();

  srv.registerPathHandler("/raw-data", handleRawData);
  srv.registerPathHandler("/called-too-late", handleTooLate);
  srv.registerPathHandler("/exceptions", handleExceptions);
  srv.registerPathHandler("/async-seizure", handleAsyncSeizure);
  srv.registerPathHandler("/seize-after-async", handleSeizeAfterAsync);

  srv.start(-1);

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






XPCOMUtils.defineLazyGetter(this, "tests", function() {
  return [
    new RawTest("localhost", PORT, data0, checkRawData),
    new RawTest("localhost", PORT, data1, checkTooLate),
    new RawTest("localhost", PORT, data2, checkExceptions),
    new RawTest("localhost", PORT, data3, checkAsyncSeizure),
    new RawTest("localhost", PORT, data4, checkSeizeAfterAsync),
  ];
});

var data0 = "GET /raw-data HTTP/1.0\r\n" +
       "\r\n";
function checkRawData(data)
{
  do_check_eq(data, "Raw data!");
}

var data1 = "GET /called-too-late HTTP/1.0\r\n" +
       "\r\n";
function checkTooLate(data)
{
  do_check_eq(LineIterator(data).next(), "too-late passed");
}

var data2 = "GET /exceptions HTTP/1.0\r\n" +
       "\r\n";
function checkExceptions(data)
{
  do_check_eq("exceptions test passed", data);
}

var data3 = "GET /async-seizure HTTP/1.0\r\n" +
       "\r\n";
function checkAsyncSeizure(data)
{
  do_check_eq(data, "async seizure passed");
}

var data4 = "GET /seize-after-async HTTP/1.0\r\n" +
       "\r\n";
function checkSeizeAfterAsync(data)
{
  do_check_eq(LineIterator(data).next(), "HTTP/1.0 200 async seizure pass");
}
