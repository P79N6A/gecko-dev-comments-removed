function testReduceOne() {
    
    
    var p = [1];
    var r = p.reducePar(function (v, p) { return v*p; });
    assertEq(r, 1);
}

if (getBuildConfiguration().parallelJS)
  testReduceOne();
