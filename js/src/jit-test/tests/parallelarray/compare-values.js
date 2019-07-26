load(libdir + "parallelarray-helpers.js");






function theTest() {
  var ints = range(0, 1024);
  var doubles = ints.map(v => v + 0.1);
  var bools = ints.map(v => (v % 2) == 0);
  var strings = ints.map(v => String(v));

  function looselyCompareToDoubles(e, i) {
    return doubles[i] == e;
  }
  print("doubles");
  compareAgainstArray(doubles, "map", looselyCompareToDoubles)
  print("bools");
  compareAgainstArray(bools, "map", looselyCompareToDoubles)
  
  
  print("strings");
  assertParallelExecWillBail(function (mode) {
    new ParallelArray(strings).map(looselyCompareToDoubles, mode)
  });
  print("ints");
  compareAgainstArray(ints, "map", looselyCompareToDoubles)

  function strictlyCompareToDoubles(e, i) {
    return doubles[i] === e;
  }
  print("doubles, strict");
  compareAgainstArray(doubles, "map", strictlyCompareToDoubles)
  print("bools, strict");
  compareAgainstArray(bools, "map", strictlyCompareToDoubles)
  print("strings, strict");
  compareAgainstArray(strings, "map", strictlyCompareToDoubles)
  print("ints, strict");
  compareAgainstArray(ints, "map", strictlyCompareToDoubles)
}

if (getBuildConfiguration().parallelJS)
  theTest();
