









































function setup()
{
  getOpenedDatabase().createTable("t1", "x TEXT");

  var stmt = createStatement("INSERT INTO t1 (x) VALUES ('a')");
  stmt.execute();

  stmt = createStatement("INSERT INTO t1 (x) VALUES ('ab')");
  stmt.execute();

  stmt = createStatement("INSERT INTO t1 (x) VALUES ('abc')");
  stmt.execute();

  stmt = createStatement("INSERT INTO t1 (x) VALUES ('abcd')");
  stmt.execute();

  stmt = createStatement("INSERT INTO t1 (x) VALUES ('acd')");
  stmt.execute();

  stmt = createStatement("INSERT INTO t1 (x) VALUES ('abd')");
  stmt.execute();

  stmt = createStatement("INSERT INTO t1 (x) VALUES ('bc')");
  stmt.execute();

  stmt = createStatement("INSERT INTO t1 (x) VALUES ('bcd')");
  stmt.execute();

  stmt = createStatement("INSERT INTO t1 (x) VALUES ('xyz')");
  stmt.execute();

  stmt = createStatement("INSERT INTO t1 (x) VALUES ('ABC')");
  stmt.execute();

  stmt = createStatement("INSERT INTO t1 (x) VALUES ('CDE')");
  stmt.execute();

  stmt = createStatement("INSERT INTO t1 (x) VALUES ('ABC abc xyz')");
  stmt.execute();
  stmt.reset();
}

function test_count()
{
  var stmt = createStatement("SELECT count(*) FROM t1;");
  do_check_true(stmt.executeStep());
  do_check_eq(stmt.getInt32(0), 12);
  stmt.reset();
}

function test_like_1()
{
  var stmt = createStatement("SELECT x FROM t1 WHERE x LIKE 'abc';");
  var solutions = ["abc", "ABC"];
  do_check_true(stmt.executeStep());
  do_check_true(solutions.indexOf(stmt.getString(0)) != -1);
  do_check_true(stmt.executeStep());
  do_check_true(solutions.indexOf(stmt.getString(0)) != -1);
  do_check_false(stmt.executeStep());
  stmt.reset();
}

function test_like_2()
{
  var stmt = createStatement("SELECT x FROM t1 WHERE x LIKE 'ABC';");
  var solutions = ["abc", "ABC"];
  do_check_true(stmt.executeStep());
  do_check_true(solutions.indexOf(stmt.getString(0)) != -1);
  do_check_true(stmt.executeStep());
  do_check_true(solutions.indexOf(stmt.getString(0)) != -1);
  do_check_false(stmt.executeStep());
  stmt.reset();
}
    
function test_like_3()
{
  var stmt = createStatement("SELECT x FROM t1 WHERE x LIKE 'aBc';");
  var solutions = ["abc", "ABC"];
  do_check_true(stmt.executeStep());
  do_check_true(solutions.indexOf(stmt.getString(0)) != -1);
  do_check_true(stmt.executeStep());
  do_check_true(solutions.indexOf(stmt.getString(0)) != -1);
  do_check_false(stmt.executeStep());
  stmt.reset();
}
   
function test_like_4()
{
  var stmt = createStatement("SELECT x FROM t1 WHERE x LIKE 'abc%';");
  var solutions = ["abc", "abcd", "ABC", "ABC abc xyz"];
  do_check_true(stmt.executeStep());
  do_check_true(solutions.indexOf(stmt.getString(0)) != -1);
  do_check_true(stmt.executeStep());
  do_check_true(solutions.indexOf(stmt.getString(0)) != -1);
  do_check_true(stmt.executeStep());
  do_check_true(solutions.indexOf(stmt.getString(0)) != -1);
  do_check_true(stmt.executeStep());
  do_check_true(solutions.indexOf(stmt.getString(0)) != -1);
  do_check_false(stmt.executeStep());
  stmt.reset();
}

function test_like_5()
{
  var stmt = createStatement("SELECT x FROM t1 WHERE x LIKE 'a_c';");
  var solutions = ["abc", "ABC"];
  do_check_true(stmt.executeStep());
  do_check_true(solutions.indexOf(stmt.getString(0)) != -1);
  do_check_true(stmt.executeStep());
  do_check_true(solutions.indexOf(stmt.getString(0)) != -1);
  do_check_false(stmt.executeStep());
  stmt.reset();
}

function test_like_6()
{
  var stmt = createStatement("SELECT x FROM t1 WHERE x LIKE 'ab%d';");
  var solutions = ["abcd", "abd"];
  do_check_true(stmt.executeStep());
  do_check_true(solutions.indexOf(stmt.getString(0)) != -1);
  do_check_true(stmt.executeStep());
  do_check_true(solutions.indexOf(stmt.getString(0)) != -1);
  do_check_false(stmt.executeStep());
  stmt.reset();
}
    
function test_like_7()
{
  var stmt = createStatement("SELECT x FROM t1 WHERE x LIKE 'a_c%';");
  var solutions = ["abc", "abcd", "ABC", "ABC abc xyz"];
  do_check_true(stmt.executeStep());
  do_check_true(solutions.indexOf(stmt.getString(0)) != -1);
  do_check_true(stmt.executeStep());
  do_check_true(solutions.indexOf(stmt.getString(0)) != -1);
  do_check_true(stmt.executeStep());
  do_check_true(solutions.indexOf(stmt.getString(0)) != -1);
  do_check_true(stmt.executeStep());
  do_check_true(solutions.indexOf(stmt.getString(0)) != -1);
  do_check_false(stmt.executeStep());
  stmt.reset();
}

function test_like_8()
{
  var stmt = createStatement("SELECT x FROM t1 WHERE x LIKE '%bcd';");
  var solutions = ["abcd", "bcd"];
  do_check_true(stmt.executeStep());
  do_check_true(solutions.indexOf(stmt.getString(0)) != -1);
  do_check_true(stmt.executeStep());
  do_check_true(solutions.indexOf(stmt.getString(0)) != -1);
  do_check_false(stmt.executeStep());
  stmt.reset();
}
    
var tests = [test_count, test_like_1, test_like_2, test_like_3, test_like_4, 
             test_like_5, test_like_6, test_like_7, test_like_8];

function run_test()
{
  setup();

  for (var i = 0; i < tests.length; i++)
    tests[i]();
    
  cleanup();
}

