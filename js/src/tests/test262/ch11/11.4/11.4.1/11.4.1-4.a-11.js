













function testcase() {
  function foo(a,b)
  {
    return (delete arguments.callee); 
  }
  var d = delete arguments.callee;
  if(d === true && arguments.callee === undefined)
    return true;
 }
runTestCase(testcase);
