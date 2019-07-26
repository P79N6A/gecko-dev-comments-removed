










function testcase()
{
  try 
  {
    eval("function eval(){};");
    return true;
  }
  catch (e) {  }  
 }
runTestCase(testcase);
