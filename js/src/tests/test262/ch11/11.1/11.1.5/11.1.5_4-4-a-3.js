
















function testcase() {
  
  var o = eval("({foo:0,foo:1});");
  return o.foo===1;
  }
runTestCase(testcase);
