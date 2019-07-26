load(libdir + "parallelarray-helpers.js");




if (getBuildConfiguration().parallelJS)
  testFilter(range(0, 617), function(i) { return (i % 2) == 0; });
