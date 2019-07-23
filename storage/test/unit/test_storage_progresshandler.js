





































function setup()
{
  var msc = getOpenedDatabase();
  msc.createTable("handler_tests", "id INTEGER PRIMARY KEY, num INTEGER");
  msc.beginTransaction();

  var stmt = createStatement("INSERT INTO handler_tests (id, num) VALUES(?1, ?2)");
  for(var i = 0; i < 100; ++i) {
    stmt.bindInt32Parameter(0, i);
    stmt.bindInt32Parameter(1, Math.floor(Math.random()*1000));
    stmt.execute();
  }
  stmt.reset();
  msc.commitTransaction();
}

var testProgressHandler = {
  calls: 0,
  abort: false,

  onProgress: function(comm) {
    ++this.calls;
    return this.abort;
  }
};

function test_handler_registration()
{
  var msc = getOpenedDatabase();
  msc.setProgressHandler(10, testProgressHandler);
}

function test_handler_return()
{
  var msc = getOpenedDatabase();
  var oldH = msc.setProgressHandler(5, testProgressHandler);
  do_check_true(oldH instanceof Ci.mozIStorageProgressHandler);
}

function test_handler_removal()
{
  var msc = getOpenedDatabase();
  msc.removeProgressHandler();
  var oldH = msc.removeProgressHandler();
  do_check_eq(oldH, null);
}

function test_handler_call()
{
  var msc = getOpenedDatabase();
  msc.setProgressHandler(50, testProgressHandler);
  
  var stmt = createStatement(
    "SELECT SUM(t1.num * t2.num) FROM handler_tests AS t1, handler_tests AS t2");
  while(stmt.executeStep());
  do_check_true(testProgressHandler.calls > 0);
}

function test_handler_abort()
{
  var msc = getOpenedDatabase();
  testProgressHandler.abort = true;
  msc.setProgressHandler(50, testProgressHandler);
  
  var stmt = createStatement(
    "SELECT SUM(t1.num * t2.num) FROM handler_tests AS t1, handler_tests AS t2");
  try {
    while(stmt.executeStep());
    do_throw("We shouldn't get here!");
  } catch (e) {
    do_check_eq(Cr.NS_ERROR_FAILURE, e.result);
    
    do_check_eq(msc.lastError, 4);
  }
}

var tests = [test_handler_registration, test_handler_return,
             test_handler_removal, test_handler_call,
             test_handler_abort];

function run_test()
{
  setup();

  for (var i = 0; i < tests.length; i++) {
    tests[i]();
  }

  cleanup();
}
