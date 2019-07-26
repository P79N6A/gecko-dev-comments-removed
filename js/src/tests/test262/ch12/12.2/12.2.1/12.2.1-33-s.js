













function testcase() {
  'use strict';

  try {
    eval('function foo() { var a, arguments, b;}');
    return false;
  }
  catch (e) {
    return (e instanceof SyntaxError);
  }
}
runTestCase(testcase);