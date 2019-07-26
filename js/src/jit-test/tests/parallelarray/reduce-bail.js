load(libdir + "parallelarray-helpers.js");







function testReduce() {
  var aCounter = 0;
  function sum(a, b) {
    var r = a + b;
    if (r == 234) 
      aCounter++;
    return r;
  }

  var array = build(4096, function() { return 1; });
  var seqResult = array.reduce(sum);
  var seqCounter = aCounter;

  aCounter = 0;
  var parray = new ParallelArray(array);
  var parResult = parray.reduce(sum);
  var parCounter = aCounter;

  assertEq(true, parCounter >= seqCounter);
  assertStructuralEq(parResult, seqResult);
}

if (getBuildConfiguration().parallelJS) testReduce();
