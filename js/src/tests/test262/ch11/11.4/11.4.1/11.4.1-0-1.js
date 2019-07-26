













function testcase() {
  var x = 1;
  var y = 2;
  var z = 3;
  
  if( (!delete x || delete y) &&
      delete delete z)
  {
    return true;
  }  
 }
runTestCase(testcase);
