load(libdir + "parallelarray-helpers.js");




if (getBuildConfiguration().parallelJS)
  compareAgainstArray(range(0, 617), "filter", function(i) { return (i % 2) == 0; });
