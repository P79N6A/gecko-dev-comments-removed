











function testcase()
{
  try 
  {
    eval('"use strict"; var x = "a\\4";');
    return false;
  }
  catch (e) {
    return (e instanceof SyntaxError);
  }
 }
runTestCase(testcase);
