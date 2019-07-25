













function test_params_enumerate()
{
  let stmt = createStatement(
    "SELECT * FROM test WHERE id IN (:a, :b, :c)"
  );

  
  let expected = ["a", "b", "c"];
  let index = 0;
  for (let name in stmt.params)
    do_check_eq(name, expected[index++]);
}

function test_params_prototype()
{
  let stmt = createStatement(
    "SELECT * FROM sqlite_master"
  );

  
  
  Object.getPrototypeOf(stmt.params).test = 2;
  do_check_eq(stmt.params.test, 2);
  stmt.finalize();
}

function test_row_prototype()
{
  let stmt = createStatement(
    "SELECT * FROM sqlite_master"
  );

  do_check_true(stmt.executeStep());

  
  
  Object.getPrototypeOf(stmt.row).test = 2;
  do_check_eq(stmt.row.test, 2);

  
  delete Object.getPrototypeOf(stmt.row).test;
  stmt.finalize();
}

function test_params_gets_sync()
{
  
  















}

function test_params_gets_async()
{
  
  















}




let tests = [
  test_params_enumerate,
  test_params_prototype,
  test_row_prototype,
  test_params_gets_sync,
  test_params_gets_async,
];
function run_test()
{
  cleanup();

  
  getOpenedDatabase().executeSimpleSQL(
    "CREATE TABLE test (" +
      "id INTEGER PRIMARY KEY " +
    ")"
  );

  
  tests.forEach(function(test) test());
}
