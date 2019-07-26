











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


  return foo.call('1') === 'string' && bar.call('1') === 'object';
 }
runTestCase(testcase);
