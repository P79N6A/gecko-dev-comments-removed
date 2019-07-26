













function testcase() {
  'use strict';

  try {
    eval('(function () {arguments = 42;})()');
    return false;
  }
  catch (e) {
    return (e instanceof SyntaxError);
  }
}
runTestCase(testcase);