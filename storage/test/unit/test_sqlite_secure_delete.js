





















































function getFileContents(aFile)
{
  let fstream = Cc["@mozilla.org/network/file-input-stream;1"].
                createInstance(Ci.nsIFileInputStream);
  fstream.init(aFile, -1, 0, 0);

  let bstream = Cc["@mozilla.org/binaryinputstream;1"].
                createInstance(Ci.nsIBinaryInputStream);
  bstream.setInputStream(fstream);
  return bstream.readBytes(bstream.available());
}




function test_delete_removes_data()
{
  const TEST_STRING = "SomeRandomStringToFind";

  let file = getTestDB();
  let db = getService().openDatabase(file);

  
  db.createTable("test", "data TEXT");
  let stmt = db.createStatement("INSERT INTO test VALUES(:data)");
  stmt.params.data = TEST_STRING;
  try {
    stmt.execute();
  }
  finally {
    stmt.finalize();
  }

  
  
  
  let contents = getFileContents(file);
  do_check_neq(-1, contents.indexOf(TEST_STRING));

  
  stmt = db.createStatement("DELETE FROM test WHERE data = :data");
  stmt.params.data = TEST_STRING;
  try {
    stmt.execute();
  }
  finally {
    stmt.finalize();
  }
  db.close();

  
  contents = getFileContents(file);
  do_check_eq(-1, contents.indexOf(TEST_STRING));

  run_next_test();
}




var tests =
[
  test_delete_removes_data,
];
let index = 0;

function run_next_test()
{
  if (index < tests.length) {
    do_test_pending();
    print("Running the next test: " + tests[index].name);
    tests[index++]();
  }

  do_test_finished();
}

function run_test()
{
  cleanup();

  do_test_pending();
  run_next_test();
}
