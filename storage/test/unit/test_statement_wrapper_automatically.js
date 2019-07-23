









































function setup()
{
  getOpenedDatabase().createTable("test", "id INTEGER PRIMARY KEY, val NONE," +
                                          "alt_val NONE");
}





















function checkVal(aActualVal, aReturnedVal)
{
  if (aActualVal instanceof Date) aActualVal = aActualVal.valueOf() * 1000.0;
  do_check_eq(aActualVal, aReturnedVal);
}




function clearTable()
{
  var stmt = createStatement("DELETE FROM test");
  stmt.execute();
  stmt.finalize();
  ensureNumRows(0);
}








function ensureNumRows(aNumRows)
{
  var stmt = createStatement("SELECT COUNT(*) AS number FROM test");
  do_check_true(stmt.step());
  do_check_eq(aNumRows, stmt.row.number);
  stmt.reset();
  stmt.finalize();
}









function insertAndCheckSingleParam(aVal)
{
  clearTable();

  var stmt = createStatement("INSERT INTO test (val) VALUES (:val)");
  stmt.params.val = aVal;
  stmt.execute();
  stmt.finalize();

  ensureNumRows(1);

  stmt = createStatement("SELECT val FROM test WHERE id = 1");
  do_check_true(stmt.step());
  checkVal(aVal, stmt.row.val);
  stmt.reset();
  stmt.finalize();
}










function insertAndCheckMultipleParams(aVal)
{
  clearTable();

  var stmt = createStatement("INSERT INTO test (val, alt_val) " +
                             "VALUES (:val, :val)");
  stmt.params.val = aVal;
  stmt.execute();
  stmt.finalize();

  ensureNumRows(1);

  stmt = createStatement("SELECT val, alt_val FROM test WHERE id = 1");
  do_check_true(stmt.step());
  checkVal(aVal, stmt.row.val);
  checkVal(aVal, stmt.row.alt_val);
  stmt.reset();
  stmt.finalize();
}








function printValDesc(aVal)
{
  try
  {
    var toSource = aVal.toSource();
  }
  catch (exc)
  {
    toSource = "";
  }
  print("Testing value: toString=" + aVal +
        (toSource ? " toSource=" + toSource : ""));
}

function run_test()
{
  setup();

  
  
  
  var vals = [
    1337,       
    3.1337,     
    "foo",      
    true,       
    null,       
    new Date(), 
  ];

  vals.forEach(function (val)
  {
    printValDesc(val);
    print("Single parameter");
    insertAndCheckSingleParam(val);
    print("Multiple parameters");
    insertAndCheckMultipleParams(val)
  });

  cleanup();
}
