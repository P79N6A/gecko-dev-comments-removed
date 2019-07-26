load(libdir + "parallelarray-helpers.js");

function testReduce() {
  
  
  
  
  
  function mul(v, p) { return v*p; }
  var array = range(1, 513);
  assertArraySeqParResultsEq(array, "reduce", mul, assertAlmostEq);
}

if (getBuildConfiguration().parallelJS)
  testReduce();
