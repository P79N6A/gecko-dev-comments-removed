











function testcase() {
  'use strict';

  try {
    eval('function foo() { arguments = 42; }; foo()');
    return false;
  }
  catch (e) {
    return (e instanceof SyntaxError);
  }
 }
runTestCase(testcase);
