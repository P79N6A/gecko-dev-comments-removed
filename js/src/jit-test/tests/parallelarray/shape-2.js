load(libdir + "eqArrayHelper.js");

function testShape() {
  
  var shape = [];
  for (var i = 0; i < 8; i++) {
    shape.push(i+1);
    var p = new ParallelArray(shape, function () { return 0; });
    
    assertEqArray(p.shape, shape);
    assertEq(p.shape !== shape, true);
  }
}

if (getBuildConfiguration().parallelJS) testShape();
