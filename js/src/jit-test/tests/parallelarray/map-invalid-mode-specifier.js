





load(libdir + "parallelarray-helpers.js");

function testMap() {
  var p = new ParallelArray(range(0, 64));
  var m = p.map(function (v) { return v+1; }, { mode: "par", expect: "fail" });
  assertEqParallelArray(m, new ParallelArray(range(1, 64)));
}

if (getBuildConfiguration().parallelJS)
  testMap();
else
  throw new Error();

