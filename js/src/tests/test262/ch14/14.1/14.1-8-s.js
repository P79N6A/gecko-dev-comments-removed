











function testcase() {

  function foo()
  {
     "bogus directive";
     "use strict";
     return (this === undefined);
  }

  return foo.call(undefined);
 }
runTestCase(testcase);
