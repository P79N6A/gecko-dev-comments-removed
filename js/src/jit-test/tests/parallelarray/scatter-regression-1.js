load(libdir + "parallelarray-helpers.js");







function testScatter() {
  var p = new ParallelArray([2,3,5,17]);
  var r = p.scatter([0,0,2,1], 9, function (x,y) { return x * y; }, 3);
  var p2 = new ParallelArray([6,17,5]);
  assertEqParallelArray(r, p2);
}

testScatter();

