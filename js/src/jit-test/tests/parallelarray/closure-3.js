load(libdir + "parallelarray-helpers.js");

function testClosureCreationAndInvocation() {
  var a = range(1, minItemsTestingThreshold+1);
  function etaadd1(v) { return (function (x) { return x+1; })(v); };
  
  compareAgainstArray(a, "map", etaadd1);
}

if (getBuildConfiguration().parallelJS)
  testClosureCreationAndInvocation();
