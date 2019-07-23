












































var tests =
  [
   new Test("http://localhost:4444/test",
            null, start_test, null),
   new Test("http://localhost:4444/sjs/qi.sjs",
            null, start_sjs_qi, null),
  ];

function run_test()
{
  var srv = createServer();

  var qi;
  try
  {
    qi = srv.identity.QueryInterface(Ci.nsIHttpServerIdentity);
  }
  catch (e)
  {
    var exstr = ("" + e).split(/[\x09\x20-\x7f\x81-\xff]+/)[0];
    do_throw("server identity didn't QI: " + exstr);
    return;
  }

  srv.registerPathHandler("/test", testHandler);
  srv.registerDirectory("/", do_get_file("data/"));
  srv.registerContentType("sjs", "sjs");
  srv.start(4444);

  runHttpTests(tests, testComplete(srv));
}




function start_test(ch, cx)
{
  do_check_eq(ch.responseStatusText, "QI Tests Passed");
  do_check_eq(ch.responseStatus, 200);
}

function start_sjs_qi(ch, cx)
{
  do_check_eq(ch.responseStatusText, "SJS QI Tests Passed");
  do_check_eq(ch.responseStatus, 200);
}


function testHandler(request, response)
{
  var exstr;
  var qid;

  response.setStatusLine(request.httpVersion, 500, "FAIL");

  var passed = false;
  try
  {
    qid = request.QueryInterface(Ci.nsIHttpRequest);
    passed = qid === request;
  }
  catch (e)
  {
    exstr = ("" + e).split(/[\x09\x20-\x7f\x81-\xff]+/)[0];
    response.setStatusLine(request.httpVersion, 500,
                           "request doesn't QI: " + exstr);
    return;
  }
  if (!passed)
  {
    response.setStatusLine(request.httpVersion, 500, "request QI'd wrongly?");
    return;
  }

  passed = false;
  try
  {
    qid = response.QueryInterface(Ci.nsIHttpResponse);
    passed = qid === response;
  }
  catch (e)
  {
    exstr = ("" + e).split(/[\x09\x20-\x7f\x81-\xff]+/)[0];
    response.setStatusLine(request.httpVersion, 500,
                           "response doesn't QI: " + exstr);
    return;
  }
  if (!passed)
  {
    response.setStatusLine(request.httpVersion, 500, "response QI'd wrongly?");
    return;
  }

  response.setStatusLine(request.httpVersion, 200, "QI Tests Passed");
}
