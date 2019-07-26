











function testcase() {
  'use strict';

  try {
    eval('function foo() { var arguments;}');
    return false;
  }
  catch (e) {
    return (e instanceof SyntaxError);
  }
 }
runTestCase(testcase);
