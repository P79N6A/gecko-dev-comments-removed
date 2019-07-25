load(libdir + "asserts.js");

function testScatterThrows() {
  
  assertThrowsInstanceOf(function () {
    var p = new ParallelArray([1,2,3,4,5]);
    var r = p.scatter([0,1,0,3,4]);
  }, Error);
  
  assertThrowsInstanceOf(function () {
    var p = new ParallelArray([1,2,3,4,5]);
    var r = p.scatter([0,1,0,3,11]);
  }, Error);
}

testScatterThrows();
