load(libdir + "parallelarray-helpers.js");

function testElement() {
  
  var p = new ParallelArray([2,2,2], function () { return 0; });
  var p0 = new ParallelArray([2,2], function () { return 0; });
  assertEqParallelArray(p[0], p0);
  
  assertEq(p[0] !== p[0], true);
  
  assertEq(p[42], undefined);
}

testElement();
