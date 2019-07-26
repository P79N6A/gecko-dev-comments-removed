













function testcase() {
  'use strict';

  try {
    eval('function foo() { var a = 42, arguments;}');
    return false;
  }
  catch (e) {
    return (e instanceof SyntaxError);
  }
}
runTestCase(testcase);