










function testcase() {

  function foo()
  {
    var x;
    'use strict';
    return (this !== undefined);
  }

  return foo.call(undefined);
 }
runTestCase(testcase);
