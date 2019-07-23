








































var srv;

function run_test()
{
  srv = createServer();

  srv.registerPathHandler("/no/setstatusline", noSetstatusline);
  srv.registerPathHandler("/http1_0", http1_0);
  srv.registerPathHandler("/http1_1", http1_1);
  srv.registerPathHandler("/invalidVersion", invalidVersion);
  srv.registerPathHandler("/invalidStatus", invalidStatus);
  srv.registerPathHandler("/invalidDescription", invalidDescription);
  srv.registerPathHandler("/crazyCode", crazyCode);
  srv.registerPathHandler("/nullVersion", nullVersion);

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






var tests = [];
var test;


function noSetstatusline(metadata, response)
{
}
test = new Test("http://localhost:4444/no/setstatusline",
                null, startNoSetStatusLine, stop);
tests.push(test);
function startNoSetStatusLine(ch, cx)
{
  checkStatusLine(ch, 1, 1, 200, "OK");
}
function stop(ch, cx, status, data)
{
  do_check_true(Components.isSuccessCode(status));
}



function http1_0(metadata, response)
{
  response.setStatusLine("1.0", 200, "OK");
}
test = new Test("http://localhost:4444/http1_0",
                null, startHttp1_0, stop);
tests.push(test);
function startHttp1_0(ch, cx)
{
  checkStatusLine(ch, 1, 0, 200, "OK");
}



function http1_1(metadata, response)
{
  response.setStatusLine("1.1", 200, "OK");
}
test = new Test("http://localhost:4444/http1_1",
                null, startHttp1_1, stop);
tests.push(test);
function startHttp1_1(ch, cx)
{
  checkStatusLine(ch, 1, 1, 200, "OK");
}



function invalidVersion(metadata, response)
{
  try
  {
    response.setStatusLine(" 1.0", 200, "FAILED");
  }
  catch (e)
  {
    response.setHeader("Passed", "true", false);
  }
}
test = new Test("http://localhost:4444/invalidVersion",
                null, startPassedTrue, stop);
tests.push(test);
function startPassedTrue(ch, cx)
{
  checkStatusLine(ch, 1, 1, 200, "OK");
  do_check_eq(ch.getResponseHeader("Passed"), "true");
}



function invalidStatus(metadata, response)
{
  try
  {
    response.setStatusLine("1.0", 1000, "FAILED");
  }
  catch (e)
  {
    response.setHeader("Passed", "true", false);
  }
}
test = new Test("http://localhost:4444/invalidStatus",
                null, startPassedTrue, stop);
tests.push(test);



function invalidDescription(metadata, response)
{
  try
  {
    response.setStatusLine("1.0", 200, "FAILED\x01");
  }
  catch (e)
  {
    response.setHeader("Passed", "true", false);
  }
}
test = new Test("http://localhost:4444/invalidDescription",
                null, startPassedTrue, stop);
tests.push(test);



function crazyCode(metadata, response)
{
  response.setStatusLine("1.1", 617, "Crazy");
}
test = new Test("http://localhost:4444/crazyCode",
                null, startCrazy, stop);
tests.push(test);
function startCrazy(ch, cx)
{
  checkStatusLine(ch, 1, 1, 617, "Crazy");
}



function nullVersion(metadata, response)
{
  response.setStatusLine(null, 255, "NULL");
}
test = new Test("http://localhost:4444/nullVersion",
                null, startNullVersion, stop);
tests.push(test);
function startNullVersion(ch, cx)
{
  
  checkStatusLine(ch, 1, 1, 255, "NULL");
}
