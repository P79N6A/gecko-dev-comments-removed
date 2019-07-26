











function testcase() {

  function foo()
  {
    'Use Strict';
     return (this !== undefined);
  }

  return foo.call(undefined);
 }
runTestCase(testcase);
