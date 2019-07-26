load(libdir + "parallelarray-helpers.js");

function test() {
  
  
  
  
  var makeadd1 = function (v) { return [v]; }
  var array = range(1, 3);
  var expected = array.map(makeadd1);
  var actual = new ParallelArray(array).map(makeadd1);
  assertStructuralEq(expected, actual);
}

if (getBuildConfiguration().parallelJS)
  test();
