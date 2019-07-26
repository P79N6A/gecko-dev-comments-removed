











function testcase()
{
  try 
  {
    eval('"use strict"; var x = "\\01z";');
    return false;
  }
  catch (e) {
    return (e instanceof SyntaxError);
  }
 }
runTestCase(testcase);
