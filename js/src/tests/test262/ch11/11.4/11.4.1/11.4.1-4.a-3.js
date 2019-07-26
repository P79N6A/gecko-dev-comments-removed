













function testcase() {
  var o = {};
  var desc = { value : 1, configurable: false }; 
  Object.defineProperty(o, "foo", desc);
  
  
  var d = delete o.foo;
  if (d === false && o.hasOwnProperty("foo") === true) {
    return true;
  }
 }
runTestCase(testcase);
