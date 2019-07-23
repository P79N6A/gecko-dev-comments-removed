




































 
function test_BrokenFile(path, shouldThrow) {
  var f = do_get_file(path);
  var uri = "abs:" + f.path;
  print(uri);
  var didThrow = false;
  try {
      Components.utils.import(uri);
  } catch (ex) {
      print("ex: " + ex);
      didThrow = true;
  }
  
  do_check_true(didThrow == shouldThrow);
}

function run_test() {
  test_BrokenFile("js/src/xpconnect/tests/unit/bogus_exports_type.jsm", true);
  test_BrokenFile("js/src/xpconnect/tests/unit/bogus_element_type.jsm", true);
}
