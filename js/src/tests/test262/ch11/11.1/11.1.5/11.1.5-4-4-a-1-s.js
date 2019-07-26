

















function testcase() {
  
  try
  {
    eval("'use strict'; ({foo:0,foo:1});");
    return false;
  }
  catch(e)
  {
    return (e instanceof SyntaxError);
  }
 }
runTestCase(testcase);
