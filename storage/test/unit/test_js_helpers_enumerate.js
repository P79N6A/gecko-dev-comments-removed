














































function test_params_enumerate()
{
  let stmt = getOpenedDatabase().createStatement(
    "SELECT * FROM test WHERE id IN (:a, :b, :c)"
  );

  
  let expected = ["a", "b", "c"];
  let index = 0;
  for (let name in stmt.params)
    do_check_eq(name, expected[index++]);
}





let tests = [
  test_params_enumerate,
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
