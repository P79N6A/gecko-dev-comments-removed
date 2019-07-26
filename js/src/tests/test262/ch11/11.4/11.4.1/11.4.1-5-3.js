










function testcase() {
  var foo = function(){};

  
  var d = delete foo;
  if(d === false && fnExists(foo))
    return true;
 }
runTestCase(testcase);
