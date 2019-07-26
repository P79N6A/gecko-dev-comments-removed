function testLength() {
  
  var shape = [];
  for (var i = 0; i < 8; i++) {
    shape.push(i+1);
    var p = new ParallelArray(shape, function () { return 0; });
    
    assertEq(p.length, shape[0]);
  }
}

if (getBuildConfiguration().parallelJS)
  testLength();
