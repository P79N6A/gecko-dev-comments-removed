











function testcase()
{
  try 
  {
    eval('"use strict"; var x = "a\\03z";');
    return false;
  }
  catch (e) {
    return (e instanceof SyntaxError);
  }
 }
runTestCase(testcase);
