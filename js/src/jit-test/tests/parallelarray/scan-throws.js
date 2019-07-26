load(libdir + "asserts.js");

function testScanThrows() {
  
  assertThrowsInstanceOf(function () {
    var p = new ParallelArray([]);
    p.scan(function (v, p) { return v*p; });
  }, Error);
  
  assertThrowsInstanceOf(function () {
    var p = new ParallelArray([1]);
    p.scan(42);
  }, TypeError);
}

testScanThrows();
