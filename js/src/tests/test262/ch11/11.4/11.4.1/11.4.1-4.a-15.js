













function testcase() {

  var a = [1,2,3]
  a.x = 10;
  var d = delete a.x;
  if( d === true && a.x === undefined)
    return true;
 }
runTestCase(testcase);
