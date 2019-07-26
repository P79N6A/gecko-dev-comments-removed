











XPCOMUtils.defineLazyGetter(this, "URL", function() {
  return "http://localhost:" + srv.identity.primaryPort;
});

var srv;

function run_test()
{
  srv = createServer();
  var sjsDir = do_get_file("data/sjs/");
  srv.registerDirectory("/", sjsDir);
  srv.registerContentType("sjs", "sjs");
  srv.start(-1);

  function done()
  {
    do_test_pending();
    srv.stop(function() { do_test_finished(); });
    do_check_eq(gStartCount, TEST_RUNS);
    do_check_true(lastPassed);
  }

  runHttpTests(tests, done);
}





var gStartCount = 0;
var lastPassed = false;


const TEST_RUNS = 250;

XPCOMUtils.defineLazyGetter(this, "tests", function() {
  var _tests = new Array(TEST_RUNS + 1);
  var _test = new Test(URL + "/thrower.sjs?throw", null, start_thrower);
  for (var i = 0; i < TEST_RUNS; i++)
    _tests[i] = _test;
  
  _tests[TEST_RUNS] = new Test(URL + "/thrower.sjs", null, start_last);
  return _tests;
});

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
