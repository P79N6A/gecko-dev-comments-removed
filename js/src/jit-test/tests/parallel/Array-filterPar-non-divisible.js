load(libdir + "parallelarray-helpers.js");




if (getBuildConfiguration().parallelJS)
  assertArraySeqParResultsEq(range(0, 617), "filter", function(i) { return (i % 2) == 0; });
