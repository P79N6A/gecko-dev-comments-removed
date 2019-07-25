









































function test_table_creation()
{
  var msc = getOpenedUnsharedDatabase();

  msc.executeSimpleSQL(
    "CREATE VIRTUAL TABLE recipe USING fts3(name, ingredients)");

  do_check_true(msc.tableExists("recipe"));
}

function test_insertion()
{
  var msc = getOpenedUnsharedDatabase();

  msc.executeSimpleSQL("INSERT INTO recipe (name, ingredients) VALUES " +
                       "('broccoli stew', 'broccoli peppers cheese tomatoes')");
  msc.executeSimpleSQL("INSERT INTO recipe (name, ingredients) VALUES " +
                       "('pumpkin stew', 'pumpkin onions garlic celery')");
  msc.executeSimpleSQL("INSERT INTO recipe (name, ingredients) VALUES " +
                       "('broccoli pie', 'broccoli cheese onions flour')");
  msc.executeSimpleSQL("INSERT INTO recipe (name, ingredients) VALUES " +
                       "('pumpkin pie', 'pumpkin sugar flour butter')");

  var stmt = msc.createStatement("SELECT COUNT(*) FROM recipe");
  stmt.executeStep();

  do_check_eq(stmt.getInt32(0), 4);

  stmt.reset();
  stmt.finalize();
}

function test_selection()
{
  var msc = getOpenedUnsharedDatabase();

  var stmt = msc.createStatement(
    "SELECT rowid, name, ingredients FROM recipe WHERE name MATCH 'pie'");

  do_check_true(stmt.executeStep());
  do_check_eq(stmt.getInt32(0), 3);
  do_check_eq(stmt.getString(1), "broccoli pie");
  do_check_eq(stmt.getString(2), "broccoli cheese onions flour");

  do_check_true(stmt.executeStep());
  do_check_eq(stmt.getInt32(0), 4);
  do_check_eq(stmt.getString(1), "pumpkin pie");
  do_check_eq(stmt.getString(2), "pumpkin sugar flour butter");

  do_check_false(stmt.executeStep());

  stmt.reset();
  stmt.finalize();
}

var tests = [test_table_creation, test_insertion, test_selection];

function run_test()
{
  
  
  
  cleanup();

  try {
    for (var i = 0; i < tests.length; i++) {
      tests[i]();
    }
  }
  
  
  
  
  finally {
    cleanup();
  }
}
