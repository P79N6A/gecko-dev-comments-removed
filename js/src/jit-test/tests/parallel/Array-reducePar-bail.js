load(libdir + "parallelarray-helpers.js");







function testReduce() {
  var aCounter = 0;
  function sum(a, b) {
    var r = a + b;
    if (r == 234) 
      aCounter++;
    return r;
  }

  
  var array = build(8 * 4096, function() { return 1; });
  var seqResult = array.reduce(sum);
  var seqCounter = aCounter;

  aCounter = 0;
  var parResult = array.reducePar(sum);
  var parCounter = aCounter;

  assertEq(true, parCounter >= seqCounter);
  assertStructuralEq(parResult, seqResult);
}

if (getBuildConfiguration().parallelJS)
  testReduce();
