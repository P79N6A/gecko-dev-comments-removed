













function testcase() {
  try {
      var o = JSON;
      var d = delete JSON;  
      if (d === true) {	    
        return true;
      }
  } finally {
    JSON = o;
  }
 }
runTestCase(testcase);
