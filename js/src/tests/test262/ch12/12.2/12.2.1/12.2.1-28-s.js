













function testcase() {
  'use strict';

  try {
    eval('function foo() { var a, arguments = 42;}');
    return false;
  }
  catch (e) {
    return (e instanceof SyntaxError);
  }
}
runTestCase(testcase);