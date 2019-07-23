






































function test_openSpecialDatabase_invalid_arg()
{
  try {
    getService().openSpecialDatabase("abcd");
    do_throw("We should not get here!");
  } catch (e) {
    print(e);
    print("e.result is " + e.result);
    do_check_eq(Cr.NS_ERROR_INVALID_ARG, e.result);
  }
}

function test_openDatabase_file_DNE()
{
  
  var db = getTestDB();
  do_check_false(db.exists());
  getService().openDatabase(db);
  do_check_true(db.exists());
}

function test_openDatabase_file_exists()
{
  
  var db = getTestDB();
  do_check_true(db.exists());
  getService().openDatabase(db);
  do_check_true(db.exists());
}

var tests = [test_openSpecialDatabase_invalid_arg, test_openDatabase_file_DNE, 
             test_openDatabase_file_exists];

function run_test()
{
  for (var i = 0; i < tests.length; i++)
    tests[i]();
    
  cleanup();
}

