










































function setup()
{
  getOpenedDatabase().createTable("t1", "x TEXT UNIQUE");

  var stmt = createStatement("INSERT INTO t1 (x) VALUES ('a')");
  stmt.execute();
  stmt.reset();
}

function test_vacuum()
{
  var stmt = createStatement("VACUUM;");
  stmt.executeStep();
  stmt.reset();
}

var tests = [test_vacuum];

function run_test()
{
  setup();

  for (var i = 0; i < tests.length; i++)
    tests[i]();
    
  cleanup();
}

