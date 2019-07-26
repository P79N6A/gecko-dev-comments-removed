













function testcase() {
  var x = 1;
  var d = eval("delete x");
  if (d === false && x === 1) {
    return true;
  }
  return false;
 }
runTestCase(testcase);
