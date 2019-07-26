load(libdir + "parallelarray-helpers.js");

function testReduce() {
  
  
  

  function mul(v, p) { return v*p; }

  
  var array = range(1, minItemsTestingThreshold+1).map(
    function(v) { return (v % 4) + 1; });
  compareAgainstArray(array, "reduce", mul, assertAlmostEq);
}

if (getBuildConfiguration().parallelJS) testReduce();
