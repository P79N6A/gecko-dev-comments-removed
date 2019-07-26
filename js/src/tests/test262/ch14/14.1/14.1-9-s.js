











function testcase() {

  function foo()
  {
     'use strict';
     "use strict";
     return (this === undefined);
  }

  return foo.call(undefined);
 }
runTestCase(testcase);
