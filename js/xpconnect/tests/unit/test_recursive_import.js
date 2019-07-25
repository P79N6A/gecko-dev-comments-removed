





































 
function run_test() {
  var scope = {};
  Components.utils.import("resource://test/recursive_importA.jsm", scope);

  
  do_check_true(scope.foo() == "foo");

  
  do_check_true(scope.bar.baz() == "baz");

  
  do_check_true(scope.bar.qux.foo() == "foo");
}
