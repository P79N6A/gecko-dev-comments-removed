








































var tests =
  [
   new Test("http://localhost:4444/throws/exception",
            null, start_throws_exception, succeeded),
   new Test("http://localhost:4444/this/file/does/not/exist/and/404s",
            null, start_nonexistent_404_fails_so_400, succeeded),
   new Test("http://localhost:4444/attempts/404/fails/so/400/fails/so/500s",
            register400Handler, start_multiple_exceptions_500, succeeded),
  ];

var srv;

function run_test()
{
  srv = createServer();

  srv.registerErrorHandler(404, throwsException);
  srv.registerPathHandler("/throws/exception", throwsException);

  srv.start(4444);

  runHttpTests(tests, testComplete(srv));
}




function checkStatusLine(channel, httpMaxVer, httpMinVer, httpCode, statusText)
{
  do_check_eq(channel.responseStatus, httpCode);
  do_check_eq(channel.responseStatusText, statusText);

  var respMaj = {}, respMin = {};
  channel.getResponseVersion(respMaj, respMin);
  do_check_eq(respMaj.value, httpMaxVer);
  do_check_eq(respMin.value, httpMinVer);
}

function start_throws_exception(ch, cx)
{
  checkStatusLine(ch, 1, 1, 500, "Internal Server Error");
}

function start_nonexistent_404_fails_so_400(ch, cx)
{
  checkStatusLine(ch, 1, 1, 400, "Bad Request");
}

function start_multiple_exceptions_500(ch, cx)
{
  checkStatusLine(ch, 1, 1, 500, "Internal Server Error");
}

function succeeded(ch, cx, status, data)
{
  do_check_true(Components.isSuccessCode(status));
}

function register400Handler(ch)
{
  srv.registerErrorHandler(400, throwsException);
}





function throwsException(metadata, response)
{
  throw "this shouldn't cause an exit...";
  do_throw("Not reached!");
}
