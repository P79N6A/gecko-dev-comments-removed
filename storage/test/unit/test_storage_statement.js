






































function setup()
{
  getOpenedDatabase().createTable("test", "id INTEGER PRIMARY KEY, name TEXT");
}

function test_parameterCount_none()
{
  var stmt = createStatement("SELECT * FROM test");
  do_check_eq(0, stmt.parameterCount);
}

function test_parameterCount_one()
{
  var stmt = createStatement("SELECT * FROM test WHERE id = ?1");
  do_check_eq(1, stmt.parameterCount);
}

function test_getParameterName()
{
  var stmt = createStatement("SELECT * FROM test WHERE id = :id");
  do_check_eq(":id", stmt.getParameterName(0));
}

function test_getParameterIndex_different()
{
  var stmt = createStatement("SELECT * FROM test WHERE id = :id OR name = :name");
  do_check_eq(0, stmt.getParameterIndex(":id"));
  do_check_eq(1, stmt.getParameterIndex(":name"));
}

function test_getParameterIndex_same()
{
  var stmt = createStatement("SELECT * FROM test WHERE id = @test OR name = @test");
  do_check_eq(0, stmt.getParameterIndex("@test"));
}

function test_columnCount()
{
  var stmt = createStatement("SELECT * FROM test WHERE id = ?1 OR name = ?2");
  do_check_eq(2, stmt.columnCount);
}

function test_getColumnName()
{
  var stmt = createStatement("SELECT name, id FROM test");
  do_check_eq("id", stmt.getColumnName(1));
  do_check_eq("name", stmt.getColumnName(0));
}

function test_state_ready()
{
  var stmt = createStatement("SELECT name, id FROM test");
  do_check_eq(Ci.mozIStorageStatement.MOZ_STORAGE_STATEMENT_READY, stmt.state);
}

function test_state_executing()
{
  var stmt = createStatement("INSERT INTO test (name) VALUES ('foo')");
  stmt.execute();
  stmt.execute();

  stmt = createStatement("SELECT name, id FROM test");
  stmt.executeStep();
  do_check_eq(Ci.mozIStorageStatement.MOZ_STORAGE_STATEMENT_EXECUTING,
              stmt.state);
  stmt.executeStep();
  do_check_eq(Ci.mozIStorageStatement.MOZ_STORAGE_STATEMENT_EXECUTING,
              stmt.state);
  stmt.reset();
  do_check_eq(Ci.mozIStorageStatement.MOZ_STORAGE_STATEMENT_READY, stmt.state);
}

var tests = [test_parameterCount_none, test_parameterCount_one,
             test_getParameterName, test_getParameterIndex_different,
             test_getParameterIndex_same, test_columnCount,
             test_getColumnName, test_state_ready, test_state_executing];

function run_test()
{
  setup();

  for (var i = 0; i < tests.length; i++)
    tests[i]();
    
  cleanup();
}

