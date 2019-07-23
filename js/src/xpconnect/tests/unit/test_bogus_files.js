




































 
function test_BrokenFile(path, shouldThrow, expectedName) {
  var f = do_get_file(path, true);
  var uri = "abs:" + f.path;
  print(uri);
  var didThrow;
  try {
    Components.utils.import(uri);
  } catch (ex) {
    var exceptionName = ex.name;
    print("ex: " + ex + "; name = " + ex.name);
    didThrow = true;
  }

  do_check_eq(didThrow, shouldThrow);
  if (didThrow)
    do_check_eq(exceptionName, expectedName);
}

function run_test() {
  test_BrokenFile("js/src/xpconnect/tests/unit/bogus_exports_type.jsm", true, "Error");
  test_BrokenFile("js/src/xpconnect/tests/unit/bogus_element_type.jsm", true, "Error");
  test_BrokenFile("js/src/xpconnect/tests/unit/non_existing.jsm", true, "NS_ERROR_FILE_NOT_FOUND");
}
