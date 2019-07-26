











function testcase() {
  'use strict';

  try {
    eval('function foo() { eval = 42; }; foo()');
    return false;
  }
  catch (e) {
    return (e instanceof SyntaxError);
  }
 }
runTestCase(testcase);
