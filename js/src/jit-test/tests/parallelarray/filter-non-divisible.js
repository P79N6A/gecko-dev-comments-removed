load(libdir + "parallelarray-helpers.js");




if (getBuildConfiguration().parallelJS)
  compareAgainstArray(range(0, minItemsTestingThreshold+17), "filter",
                      function(i) { return (i % 2) == 0; });
