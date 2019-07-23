








































const PORT = 4444;

var srv;

function run_test()
{
  srv = createServer();
  var sjsDir = do_get_file("netwerk/test/httpserver/test/data/sjs/");
  srv.registerDirectory("/", sjsDir);
  srv.registerContentType("sjs", "sjs");
  srv.registerPathHandler("/path-handler", pathHandler);
  srv.start(PORT);

  function done()
  {
    do_check_eq(srv.getState("foopy"), "done!");
    srv.stop();
  }

  runHttpTests(tests, done);
}






function pathHandler(request, response)
{
  response.setHeader("Cache-Control", "no-cache", false);

  var oldval = srv.getState("foopy");
  response.setHeader("X-Old-Value", oldval, false);
  var newval = "back to SJS!";
  srv.setState("foopy", newval);
  response.setHeader("X-New-Value", newval, false);
}






var test;
var tests = [];


function expectValues(ch, oldval, newval)
{
  do_check_eq(ch.getResponseHeader("X-Old-Value"), oldval);
  do_check_eq(ch.getResponseHeader("X-New-Value"), newval);
}


test = new Test("http://localhost:4444/state.sjs?1",
                null, start_initial);
tests.push(test);

function start_initial(ch, cx)
{
  expectValues(ch, "", "first set!");
}


test = new Test("http://localhost:4444/state.sjs?2",
                null, start_next);
tests.push(test);

function start_next(ch, cx)
{
  expectValues(ch, "first set!", "changed!");
}


test = new Test("http://localhost:4444/path-handler",
                null, start_handler);
tests.push(test);

function start_handler(ch, cx)
{
  expectValues(ch, "changed!", "back to SJS!");
}


test = new Test("http://localhost:4444/state.sjs?3",
                null, start_last);
tests.push(test);

function start_last(ch, cx)
{
  expectValues(ch, "back to SJS!", "done!");
}
