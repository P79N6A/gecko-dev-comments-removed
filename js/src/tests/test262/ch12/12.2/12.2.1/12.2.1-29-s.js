













function testcase() {
  'use strict';

  try {
    eval('function foo() { var eval, a = 42;}');
    return false;
  }
  catch (e) {
    return (e instanceof SyntaxError);
  }
}
runTestCase(testcase);