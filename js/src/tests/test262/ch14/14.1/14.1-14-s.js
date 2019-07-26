











function testcase() {

  function foo()
  {
    "another directive"
    "use strict" ;
    return (this === undefined);
  }

  return foo.call(undefined);
 }
runTestCase(testcase);
