load(libdir + "asserts.js");

function testReduceThrows() {
  
  assertThrowsInstanceOf(function () {
    var p = new ParallelArray([]);
    p.reduce(function (v, p) { return v*p; });
  }, Error);
  
  assertThrowsInstanceOf(function () {
    var p = new ParallelArray([1]);
    p.reduce(42);
  }, TypeError);
}

testReduceThrows();
