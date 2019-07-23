








































const PORT = 4444;

function run_test()
{
  var srv;

  srv = createServer();
  srv.registerPathHandler("/path-handler", pathHandler);
  srv.start(PORT);

  runHttpTests(tests, testComplete(srv));
}






function pathHandler(request, response)
{
  response.setHeader("Cache-Control", "no-cache", false);

  response.setHeader("Proxy-Authenticate", "First line 1", true);
  response.setHeader("Proxy-Authenticate", "Second line 1", true);
  response.setHeader("Proxy-Authenticate", "Third line 1", true);

  response.setHeader("WWW-Authenticate", "Not merged line 1", false);
  response.setHeader("WWW-Authenticate", "Not merged line 2", true);

  response.setHeader("WWW-Authenticate", "First line 2", false);
  response.setHeader("WWW-Authenticate", "Second line 2", true);
  response.setHeader("WWW-Authenticate", "Third line 2", true);

  response.setHeader("X-Single-Header-Merge", "Single 1", true);
  response.setHeader("X-Single-Header-Merge", "Single 2", true);
}





var tests = [
  new Test("http://localhost:4444/path-handler",
           null, check)];

function check(ch, cx)
{
  var headerValue;

  headerValue = ch.getResponseHeader("Proxy-Authenticate");
  do_check_eq(headerValue, "First line 1\nSecond line 1\nThird line 1");
  headerValue = ch.getResponseHeader("WWW-Authenticate");
  do_check_eq(headerValue, "First line 2\nSecond line 2\nThird line 2");
  headerValue = ch.getResponseHeader("X-Single-Header-Merge");
  do_check_eq(headerValue, "Single 1,Single 2");
}
