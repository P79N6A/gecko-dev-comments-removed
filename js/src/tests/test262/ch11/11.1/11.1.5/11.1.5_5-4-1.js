















function testcase() {

  var o = {foo : 1};
  var desc = Object.getOwnPropertyDescriptor(o,"foo");
  if(desc.value === 1 &&
     desc.writable === true &&
     desc.enumerable === true &&
     desc.configurable === true)
    return true;
 }
runTestCase(testcase);
