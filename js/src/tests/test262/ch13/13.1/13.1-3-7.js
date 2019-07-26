










function testcase()
{
  try 
  {
    eval("function arguments (){};");
    return true;
  }
  catch (e) {  }  
 }
runTestCase(testcase);
