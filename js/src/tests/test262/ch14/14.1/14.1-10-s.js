











function testcase() {

  function foo()
  {
     "use strict";
     "bogus directive";
     return (this === undefined);
  }

  return foo.call(undefined);
 }
runTestCase(testcase);
