






































function setup()
{
  getOpenedDatabase().createTable("test", "id INTEGER PRIMARY KEY, name TEXT," +
                                          "number REAL, nuller NULL, blobber BLOB");
  
  var stmt = createStatement("INSERT INTO test (name, number, blobber) " +
                             "VALUES (?1, ?2, ?3)");
  stmt.bindUTF8StringParameter(0, "foo");
  stmt.bindDoubleParameter(1, 2.34);
  stmt.bindBlobParameter(2, [], 0);
  stmt.execute();
  
  stmt.bindStringParameter(0, "");
  stmt.bindDoubleParameter(1, 1.23);
  stmt.bindBlobParameter(2, [1, 2], 2);
  stmt.execute();

  stmt.reset();
}

function test_getIsNull_for_null()
{
  var stmt = createStatement("SELECT nuller, blobber FROM test WHERE id = ?1");
  stmt.bindInt32Parameter(0, 1);
  do_check_true(stmt.executeStep());
  
  do_check_true(stmt.getIsNull(0)); 
  do_check_true(stmt.getIsNull(1)); 
  stmt.reset();
}

function test_getIsNull_for_non_null()
{
  var stmt = createStatement("SELECT name, blobber FROM test WHERE id = ?1");
  stmt.bindInt32Parameter(0, 2);
  do_check_true(stmt.executeStep());

  do_check_false(stmt.getIsNull(0));
  do_check_false(stmt.getIsNull(1));
  stmt.reset();
}

function test_value_type_null()
{
  var stmt = createStatement("SELECT nuller FROM test WHERE id = ?1");
  stmt.bindInt32Parameter(0, 1);
  do_check_true(stmt.executeStep());

  do_check_eq(Ci.mozIStorageValueArray.VALUE_TYPE_NULL,
              stmt.getTypeOfIndex(0));
  stmt.reset();
}

function test_value_type_integer()
{
  var stmt = createStatement("SELECT id FROM test WHERE id = ?1");
  stmt.bindInt32Parameter(0, 1);
  do_check_true(stmt.executeStep());

  do_check_eq(Ci.mozIStorageValueArray.VALUE_TYPE_INTEGER,
              stmt.getTypeOfIndex(0));
  stmt.reset();
}

function test_value_type_float()
{
  var stmt = createStatement("SELECT number FROM test WHERE id = ?1");
  stmt.bindInt32Parameter(0, 1);
  do_check_true(stmt.executeStep());

  do_check_eq(Ci.mozIStorageValueArray.VALUE_TYPE_FLOAT,
              stmt.getTypeOfIndex(0));
  stmt.reset();
}

function test_value_type_text()
{
  var stmt = createStatement("SELECT name FROM test WHERE id = ?1");
  stmt.bindInt32Parameter(0, 1);
  do_check_true(stmt.executeStep());

  do_check_eq(Ci.mozIStorageValueArray.VALUE_TYPE_TEXT,
              stmt.getTypeOfIndex(0));
  stmt.reset();
}

function test_value_type_blob()
{
  var stmt = createStatement("SELECT blobber FROM test WHERE id = ?1");
  stmt.bindInt32Parameter(0, 2);
  do_check_true(stmt.executeStep());

  do_check_eq(Ci.mozIStorageValueArray.VALUE_TYPE_BLOB,
              stmt.getTypeOfIndex(0));
  stmt.reset();
}

function test_numEntries_one()
{
  var stmt = createStatement("SELECT blobber FROM test WHERE id = ?1");
  stmt.bindInt32Parameter(0, 2);
  do_check_true(stmt.executeStep());

  do_check_eq(1, stmt.numEntries);
  stmt.reset();
}

function test_numEntries_all()
{
  var stmt = createStatement("SELECT * FROM test WHERE id = ?1");
  stmt.bindInt32Parameter(0, 2);
  do_check_true(stmt.executeStep());

  do_check_eq(5, stmt.numEntries);
  stmt.reset();
}

function test_getInt()
{
  var stmt = createStatement("SELECT id FROM test WHERE id = ?1");
  stmt.bindInt32Parameter(0, 2);
  do_check_true(stmt.executeStep());

  do_check_eq(2, stmt.getInt32(0));
  do_check_eq(2, stmt.getInt64(0));
  stmt.reset();
}

function test_getDouble()
{
  var stmt = createStatement("SELECT number FROM test WHERE id = ?1");
  stmt.bindInt32Parameter(0, 2);
  do_check_true(stmt.executeStep());

  do_check_eq(1.23, stmt.getDouble(0));
  stmt.reset();
}

function test_getUTF8String()
{
  var stmt = createStatement("SELECT name FROM test WHERE id = ?1");
  stmt.bindInt32Parameter(0, 1);
  do_check_true(stmt.executeStep());

  do_check_eq("foo", stmt.getUTF8String(0));
  stmt.reset();
}

function test_getString()
{
  var stmt = createStatement("SELECT name FROM test WHERE id = ?1");
  stmt.bindInt32Parameter(0, 2);
  do_check_true(stmt.executeStep());

  do_check_eq("", stmt.getString(0));
  stmt.reset();
}

function test_getBlob()
{
  var stmt = createStatement("SELECT blobber FROM test WHERE id = ?1");
  stmt.bindInt32Parameter(0, 2);
  do_check_true(stmt.executeStep());

  var count = { value: 0 };
  var arr = { value: null };
  stmt.getBlob(0, count, arr);
  do_check_eq(2, count.value);
  do_check_eq(1, arr.value[0]);
  do_check_eq(2, arr.value[1]);
  stmt.reset();
}

var tests = [test_getIsNull_for_null, test_getIsNull_for_non_null,
             test_value_type_null, test_value_type_integer,
             test_value_type_float, test_value_type_text, test_value_type_blob,
             test_numEntries_one, test_numEntries_all, test_getInt,
             test_getDouble, test_getUTF8String, test_getString, test_getBlob];

function run_test()
{
  setup();

  for (var i = 0; i < tests.length; i++)
    tests[i]();
    
  cleanup();
}

