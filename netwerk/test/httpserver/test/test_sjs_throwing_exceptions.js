












































const PORT = 4444;

function run_test()
{
  var srv = createServer();
  var sjsDir = do_get_file("data/sjs/");
  srv.registerDirectory("/", sjsDir);
  srv.registerContentType("sjs", "sjs");
  srv.start(PORT);

  function done()
  {
    srv.stop();
    do_check_eq(gStartCount, TEST_RUNS);
    do_check_true(lastPassed);
  }

  runHttpTests(tests, done);
}





var gStartCount = 0;
var lastPassed = false;


const TEST_RUNS = 250;

var test = new Test("http://localhost:4444/thrower.sjs?throw",
                    null, start_thrower);

var tests = new Array(TEST_RUNS + 1);
for (var i = 0; i < TEST_RUNS; i++)
  tests[i] = test;


tests[TEST_RUNS] = new Test("http://localhost:4444/thrower.sjs",
                            null, start_last);

function start_thrower(ch, cx)
{
  do_check_eq(ch.responseStatus, 500);
  do_check_false(ch.requestSucceeded);

  gStartCount++;
}

function start_last(ch, cx)
{
  do_check_eq(ch.responseStatus, 200);
  do_check_true(ch.requestSucceeded);

  do_check_eq(ch.getResponseHeader("X-Test-Status"), "PASS");

  lastPassed = true;
}
