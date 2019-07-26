













function testcase() {

  var a = [1,2,3]
  a.x = 10;
  var d = delete a.length
  if(d === false && a.length === 3)
    return true;
 }
runTestCase(testcase);
