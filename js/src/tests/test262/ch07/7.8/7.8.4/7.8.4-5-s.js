











function testcase()
{
  try 
  {
    eval('"use strict"; var x = "\\00a";');
    return false;
  }
  catch (e) {
    return (e instanceof SyntaxError);
  }
 }
runTestCase(testcase);
