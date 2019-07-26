













function testcase() {
  'use strict';
  var s = eval;
  s('var arguments;');
  return true;
}
runTestCase(testcase);