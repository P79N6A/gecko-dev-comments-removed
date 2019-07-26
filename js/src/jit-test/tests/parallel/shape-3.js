load(libdir + "eqArrayHelper.js");

function testInfer() {
  
  var p0 = new ParallelArray([0,1]);
  var p1 = new ParallelArray([2,3]);
  var p = new ParallelArray([p0, p1]);
  assertEqArray(p.shape, [2,2]);
  var p0 = new ParallelArray([0,1]);
  var p1 = new ParallelArray([2]);
  var p = new ParallelArray([p0, p1]);
  assertEqArray(p.shape, [2]);
}



