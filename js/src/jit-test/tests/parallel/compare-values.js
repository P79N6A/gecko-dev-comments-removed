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
  assertArraySeqParResultsEq(doubles, "map", looselyCompareToDoubles);
  print("bools");
  assertArraySeqParResultsEq(bools, "map", looselyCompareToDoubles);
  
  
  print("strings");
  assertParallelExecWillBail(function (mode) {
    strings.mapPar(looselyCompareToDoubles, mode)
  });
  print("ints");
  assertArraySeqParResultsEq(ints, "map", looselyCompareToDoubles);

  function strictlyCompareToDoubles(e, i) {
    return doubles[i] === e;
  }
  print("doubles, strict");
  assertArraySeqParResultsEq(doubles, "map", strictlyCompareToDoubles);
  print("bools, strict");
  assertArraySeqParResultsEq(bools, "map", strictlyCompareToDoubles);
  print("strings, strict");
  assertArraySeqParResultsEq(strings, "map", strictlyCompareToDoubles);
  print("ints, strict");
  assertArraySeqParResultsEq(ints, "map", strictlyCompareToDoubles);
}

if (getBuildConfiguration().parallelJS)
  theTest();
