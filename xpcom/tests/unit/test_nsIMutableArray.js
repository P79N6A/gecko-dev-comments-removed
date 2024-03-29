



const Ci = Components.interfaces;
const Cr = Components.results;
const Cc = Components.classes;
const CC = Components.Constructor;

var MutableArray = CC("@mozilla.org/array;1", "nsIMutableArray");
var SupportsString = CC("@mozilla.org/supports-string;1", "nsISupportsString");

function create_n_element_array(n)
{
  var arr = new MutableArray();
  for (let i=0; i<n; i++) {
    let str = new SupportsString();
    str.data = "element " + i;
    arr.appendElement(str, false);
  }
  return arr;
}

function test_appending_null_actually_inserts()
{
  var arr = new MutableArray();
  do_check_eq(0, arr.length);
  arr.appendElement(null, false);
  do_check_eq(1, arr.length);
}

function test_object_gets_appended()
{
  var arr = new MutableArray();
  var str = new SupportsString();
  str.data = "hello";
  arr.appendElement(str, false);
  do_check_eq(1, arr.length);
  var obj = arr.queryElementAt(0, Ci.nsISupportsString);
  do_check_eq(str, obj);
}

function test_insert_at_beginning()
{
  var arr = create_n_element_array(5);
  
  do_check_eq(5, arr.length);
  var str = new SupportsString();
  str.data = "hello";
  arr.insertElementAt(str, 0, false);
  do_check_eq(6, arr.length);
  var obj = arr.queryElementAt(0, Ci.nsISupportsString);
  do_check_eq(str, obj);
  
  for (let i=1; i<arr.length; i++) {
    let obj = arr.queryElementAt(i, Ci.nsISupportsString);
    do_check_eq("element " + (i-1), obj.data);
  }
}

function test_replace_element()
{
  var arr = create_n_element_array(5);
  
  do_check_eq(5, arr.length);
  var str = new SupportsString();
  str.data = "hello";
  
  arr.replaceElementAt(str, 0, false);
  do_check_eq(5, arr.length);
  var obj = arr.queryElementAt(0, Ci.nsISupportsString);
  do_check_eq(str, obj);
  
  arr.replaceElementAt(str, arr.length - 1, false);
  do_check_eq(5, arr.length);
  obj = arr.queryElementAt(arr.length - 1, Ci.nsISupportsString);
  do_check_eq(str, obj);
  
  arr.replaceElementAt(str, 9, false);
  do_check_eq(10, arr.length);
  obj = arr.queryElementAt(9, Ci.nsISupportsString);
  do_check_eq(str, obj);
  
}

function test_clear()
{
  var arr = create_n_element_array(5);
  
  do_check_eq(5, arr.length);
  arr.clear();
  do_check_eq(0, arr.length);
}

function test_enumerate()
{
  var arr = create_n_element_array(5);
  do_check_eq(5, arr.length);
  var en = arr.enumerate();
  var i = 0;
  while (en.hasMoreElements()) {
    let str = en.getNext();
    do_check_true(str instanceof Ci.nsISupportsString);
    do_check_eq(str.data, "element " + i);
    i++;
  }
  do_check_eq(arr.length, i);
}

var tests = [
  test_appending_null_actually_inserts,
  test_object_gets_appended,
  test_insert_at_beginning,
  test_replace_element,
  test_clear,
  test_enumerate,
];

function run_test() {
  for (var i = 0; i < tests.length; i++)
    tests[i]();
}
