













function testcase() {
  'use strict';
  var s = eval;
  s('arguments = 42;');
  return true;
}
runTestCase(testcase);