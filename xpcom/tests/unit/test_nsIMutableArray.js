




































const Ci = Components.interfaces;
const Cr = Components.results;
const Cc = Components.classes;

function test_appending_null_actually_inserts()
{
  var arr = Cc["@mozilla.org/array;1"].createInstance(Ci.nsIMutableArray);
  do_check_eq(0, arr.length);
  arr.appendElement(null, false);
  do_check_eq(1, arr.length);
}

var tests = [
  test_appending_null_actually_inserts,
];

function run_test() {
  for (var i = 0; i < tests.length; i++)
    tests[i]();
}
