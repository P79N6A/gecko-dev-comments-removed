load(libdir + "parallelarray-helpers.js");

function testElement() {
  
  var p = new ParallelArray([2,2,2], function () { return 0; });
  var p0 = new ParallelArray([2,2], function () { return 0; });
  assertEqParallelArray(p[0], p0);
  
  assertEq(p[0] !== p[0], true);
  
  assertEq(p[42], undefined);
  
  var pp = new ParallelArray([0,0], function() { return 0; });
  assertEq(pp[2], undefined);
  var pp2 = new ParallelArray([2,0], function() { return 0; });
  assertEqParallelArray(pp2[0], new ParallelArray());
}

testElement();
