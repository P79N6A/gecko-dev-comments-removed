














































function test_params_prototype()
{
  let stmt = getOpenedDatabase().createStatement(
    "SELECT * FROM sqlite_master"
  );

  
  
  stmt.params.__proto__.test = 2;
  do_check_eq(stmt.params.test, 2);
  stmt.finalize();
}

function test_row_prototype()
{
  
  getOpenedDatabase().createTable("test_table", "id INTEGER PRIMARY KEY");

  let stmt = getOpenedDatabase().createStatement(
    "SELECT * FROM sqlite_master"
  );

  do_check_true(stmt.executeStep());

  
  
  stmt.row.__proto__.test = 2;
  do_check_eq(stmt.row.test, 2);
  stmt.finalize();
}




let tests = [
  test_params_prototype,
  test_row_prototype,
];
function run_test()
{
  cleanup();

  
  tests.forEach(function(test) test());
}
