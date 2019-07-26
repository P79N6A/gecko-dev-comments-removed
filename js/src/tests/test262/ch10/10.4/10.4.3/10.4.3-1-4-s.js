











function testcase() {

  function foo()
  {
    'use strict';
    return typeof(this);
  }

  function bar()
  {
    return typeof(this);
  }


  return foo.call(true) === 'boolean' && bar.call(true) === 'object';
 }
runTestCase(testcase);
