load(libdir + "parallelarray-helpers.js");

function testMap() {
  
  var p = new ParallelArray([2,2], function (i,j) { return i+j; });
  var m = p.map(function(x) { return x; });
  var p2 = new ParallelArray(p);
  assertEqParallelArray(m, p2);
}

testMap();

