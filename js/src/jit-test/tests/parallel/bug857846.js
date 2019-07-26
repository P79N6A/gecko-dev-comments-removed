function testNegativeZeroScatter() {
  
  var p = new ParallelArray([0]);
  var r = p.scatter([-0], 0, undefined, 1);
}

if (getBuildConfiguration().parallelJS) {
  testNegativeZeroScatter();
}
