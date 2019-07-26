load(libdir + "asserts.js");

function testScatterThrows() {
  var p = new ParallelArray([1,2,3,4,5]);

  
  assertThrowsInstanceOf(function () {
    var r = p.scatter([0,1,0,3,4]);
  }, Error);
  
  assertThrowsInstanceOf(function () {
    var r = p.scatter([0,1,0,3,11]);
  }, Error);

  assertThrowsInstanceOf(function () {
    p.scatter([-1,1,0,3,4], 9, function (a,b) { return a+b; }, 10);
  }, TypeError);
  assertThrowsInstanceOf(function () {
    p.scatter([0,1,0,3,4], 9, function (a,b) { return a+b; }, -1);
  }, RangeError);
  assertThrowsInstanceOf(function () {
    p.scatter([0,1,0,3,4], 9, function (a,b) { return a+b; }, 0xffffffff + 1);
  }, RangeError);
  assertThrowsInstanceOf(function () {
    p.scatter({ length: 0xffffffff + 1 }, 9, function (a,b) { return a+b; }, 10);
  }, RangeError);

}

testScatterThrows();
