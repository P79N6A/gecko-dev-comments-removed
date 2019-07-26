

function testFlattenFlat() {
  
  var p = new ParallelArray([1]);
  var f = p.flatten();
}

if (getBuildConfiguration().parallelJS)
  testFlattenFlat();
else
  throw new Error();
