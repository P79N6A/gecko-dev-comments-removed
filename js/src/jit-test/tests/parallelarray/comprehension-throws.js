load(libdir + "asserts.js");

function buildComprehension() {
  
  assertThrowsInstanceOf(function () {
    var p = new ParallelArray([2,2], undefined);
  }, TypeError);
  assertThrowsInstanceOf(function () {
    var p = new ParallelArray(/x/, /x/);
  }, TypeError);
}

buildComprehension();
