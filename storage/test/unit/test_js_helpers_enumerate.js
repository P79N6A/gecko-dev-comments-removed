














































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

function test_row_enumerate()
{
  var db = getOpenedDatabase();
  let stmt = createStatement("INSERT INTO test (driver, car) VALUES (:driver, :car)");
  stmt.params.driver = "David";
  stmt.params.car = "Fiat 500";
  try {
    stmt.execute();
  }
  finally {
    stmt.finalize();
  }

  stmt = createStatement("SELECT driver, car FROM test WHERE driver = :driver");
  stmt.params.driver = "David";

  try {
    do_check_true(stmt.executeStep());
    let expected = ["driver", "car"];
    let index = 0;
    for (let colName in stmt.row)
      do_check_eq(colName, expected[index++]);
  }
  finally {
    stmt.finalize();
  }
}




let tests = [
  test_params_enumerate,
  test_row_enumerate,
];
function run_test()
{
  cleanup();

  
  getOpenedDatabase().executeSimpleSQL(
    "CREATE TABLE test (" +
      "id INTEGER PRIMARY KEY, " +
      "driver VARCHAR(32) NULL, " +
      "car VARCHAR(32) NULL" +
    ")"
  );

  
  tests.forEach(function(test) test());
}
