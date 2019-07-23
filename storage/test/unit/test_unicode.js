






































const LATIN1_AE = "\xc6"; 
const LATIN1_ae = "\xe6";  

function setup()
{
  getOpenedDatabase().createTable("test", "id INTEGER PRIMARY KEY, name TEXT");

  var stmt = createStatement("INSERT INTO test (name, id) VALUES (?1, ?2)");
  stmt.bindStringParameter(0, LATIN1_AE);
  stmt.bindInt32Parameter(1, 1);
  stmt.execute();
  stmt.bindStringParameter(0, "A");
  stmt.bindInt32Parameter(1, 2);
  stmt.execute();
  stmt.bindStringParameter(0, "b");
  stmt.bindInt32Parameter(1, 3);
  stmt.execute();
  stmt.bindStringParameter(0, LATIN1_ae);
  stmt.bindInt32Parameter(1, 4);
  stmt.execute();
}

function test_upper_ascii()
{
  var stmt = createStatement("SELECT name, id FROM test WHERE name = upper('a')");
  do_check_true(stmt.executeStep());
  do_check_eq("A", stmt.getString(0));
  do_check_eq(2, stmt.getInt32(1));
  stmt.reset();
}

function test_upper_non_ascii()
{
  var stmt = createStatement("SELECT name, id FROM test WHERE name = upper(?1)");
  stmt.bindStringParameter(0, LATIN1_ae);
  do_check_true(stmt.executeStep());
  do_check_eq(LATIN1_AE, stmt.getString(0));
  do_check_eq(1, stmt.getInt32(1));
  stmt.reset();
}

function test_lower_ascii()
{
  var stmt = createStatement("SELECT name, id FROM test WHERE name = lower('B')");
  do_check_true(stmt.executeStep());
  do_check_eq("b", stmt.getString(0));
  do_check_eq(3, stmt.getInt32(1));
  stmt.reset();
}

function test_lower_non_ascii()
{
  var stmt = createStatement("SELECT name, id FROM test WHERE name = lower(?1)");
  stmt.bindStringParameter(0, LATIN1_AE);
  do_check_true(stmt.executeStep());
  do_check_eq(LATIN1_ae, stmt.getString(0));
  do_check_eq(4, stmt.getInt32(1));
  stmt.reset();
}

function test_like_search_different()
{
  var stmt = createStatement("SELECT COUNT(*) FROM test WHERE name LIKE ?1");
  stmt.bindStringParameter(0, LATIN1_AE);
  do_check_true(stmt.executeStep());
  do_check_eq(2, stmt.getInt32(0));
}

function test_like_search_same()
{
  var stmt = createStatement("SELECT COUNT(*) FROM test WHERE name LIKE ?1");
  stmt.bindStringParameter(0, LATIN1_ae);
  do_check_true(stmt.executeStep());
  do_check_eq(2, stmt.getInt32(0));
}

var tests = [test_upper_ascii, test_upper_non_ascii, test_lower_ascii,
             test_lower_non_ascii, test_like_search_different,
             test_like_search_same];

function run_test()
{
  setup();

  for (var i = 0; i < tests.length; i++)
    tests[i]();
    
  cleanup();
}

