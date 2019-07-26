










function testcase() {
  
  function foo(a,b,c)
  {
    arguments[0] = 1; arguments[1] = 'str'; arguments[2] = 2.1;
    if(1 === a && 'str' === b && 2.1 === c)
      return true;   
  }
  return foo(10,'sss',1);
 }
runTestCase(testcase);
