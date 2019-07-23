






































function setup()
{
  getOpenedDatabase().createTable("test", "id INTEGER PRIMARY KEY, name TEXT," +
                                          "alt_name TEXT");
}

var wrapper = new Components.Constructor("@mozilla.org/storage/statement-wrapper;1",
                                         Ci.mozIStorageStatementWrapper,
                                         "initialize");


createStatement = function(aSQL) {
  return new wrapper(getOpenedDatabase().createStatement(aSQL));
}

function test_binding_params()
{
  var stmt = createStatement("INSERT INTO test (name) VALUES (:name)");

  const name = "foo";
  stmt.params.name = name;
  stmt.execute();

  stmt = createStatement("SELECT COUNT(*) AS number FROM test");
  do_check_true(stmt.step());
  do_check_eq(1, stmt.row.number);
  stmt.reset();

  stmt = createStatement("SELECT name FROM test WHERE id = 1");
  do_check_true(stmt.step());
  do_check_eq(name, stmt.row.name);
  stmt.reset();
}

function test_binding_multiple_params()
{
  var stmt = createStatement("INSERT INTO test (name, alt_name)" +
                             "VALUES (:name, :name)");
  const name = "me";
  stmt.params.name = name;
  stmt.execute();

  stmt = createStatement("SELECT COUNT(*) AS number FROM test");
  do_check_true(stmt.step());
  do_check_eq(2, stmt.row.number);
  stmt.reset();

  stmt = createStatement("SELECT name, alt_name FROM test WHERE id = 2");
  do_check_true(stmt.step());
  do_check_eq(name, stmt.row.name);
  do_check_eq(name, stmt.row.alt_name);
  stmt.reset();
}

var tests = [test_binding_params, test_binding_multiple_params];

function run_test()
{
  setup();

  for (var i = 0; i < tests.length; i++)
    tests[i]();
    
  cleanup();
}

