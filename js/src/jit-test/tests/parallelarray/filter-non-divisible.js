load(libdir + "parallelarray-helpers.js");




testFilter(range(0, 617), function(i) { return (i % 2) == 0; });
