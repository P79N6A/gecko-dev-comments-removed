













function testcase() {
  var o = new Object();
  o.x = 1;
  var d;
  with(o)
  {
    d = delete o;
  }
  if (d === false && typeof(o) === 'object' && o.x === 1) {
    return true;
  }
  return false;
 }
runTestCase(testcase);
