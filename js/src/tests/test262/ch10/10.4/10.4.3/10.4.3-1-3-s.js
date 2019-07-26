











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
  return foo.call(undefined) === 'undefined' && bar.call() === 'object';
 }
runTestCase(testcase);
