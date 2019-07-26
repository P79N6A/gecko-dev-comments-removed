














function testcase() {
  'use strict';

  var o = {};
  var desc = { value : 1 }; 
  Object.defineProperty(o, "foo", desc);
  
  
  try {
    delete o.foo;
    return false;
  }
  catch (e) {
    return (e instanceof TypeError);
  }
 }
runTestCase(testcase);
