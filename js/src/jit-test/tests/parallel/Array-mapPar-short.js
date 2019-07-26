load(libdir + "parallelarray-helpers.js");

function test() {
  
  
  
  var makeadd1 = function (v) { return [v]; }
  assertArraySeqParResultsEq(range(1, 3), "map", makeadd1);
}

if (getBuildConfiguration().parallelJS)
  test();
