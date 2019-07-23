






































function test_openUnsharedDatabase_file_DNE()
{
  
  var db = getTestDB();
  do_check_false(db.exists());
  getService().openUnsharedDatabase(db);
  do_check_true(db.exists());
}

function test_openUnsharedDatabase_file_exists()
{
  
  var db = getTestDB();
  do_check_true(db.exists());
  getService().openUnsharedDatabase(db);
  do_check_true(db.exists());
}

var tests = [test_openUnsharedDatabase_file_DNE,
             test_openUnsharedDatabase_file_exists];

function run_test()
{
  for (var i = 0; i < tests.length; i++)
    tests[i]();
    
  cleanup();
}

