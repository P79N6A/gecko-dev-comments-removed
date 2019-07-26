load(libdir + "asserts.js");

function buildComprehension() {
  
  assertThrowsInstanceOf(function () {
    var p = new ParallelArray([2,2], undefined);
  }, TypeError);
  assertThrowsInstanceOf(function () {
    var p = new ParallelArray(2, /x/);
  }, TypeError);
  assertThrowsInstanceOf(function () {
    var p = new ParallelArray(/x/, /x/);
  }, TypeError);
  assertThrowsInstanceOf(function () {
    new ParallelArray([0xffffffff + 1], function() { return 0; });
  }, RangeError);
  assertThrowsInstanceOf(function () {
    new ParallelArray(0xffffffff + 1, function() { return 0; });
  }, RangeError);
  assertThrowsInstanceOf(function () {
    new ParallelArray([0xfffff, 0xffff], function() { return 0; });
  }, RangeError);
}




