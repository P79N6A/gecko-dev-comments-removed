










function testcase() {
  
  var desc = Object.getOwnPropertyDescriptor(arguments,"length");
  if(desc.configurable === true &&
     desc.enumerable === false &&
     desc.writable === true )
    return true;
 }
runTestCase(testcase);
