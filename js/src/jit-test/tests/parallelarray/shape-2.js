function testShape() {
  
  var shape = [];
  for (var i = 0; i < 8; i++) {
    shape.push(i+1);
    var p = new ParallelArray(shape, function () { return 0; });
    
    assertEq(p.shape, p.shape);
    assertEq(p.shape !== shape, true);
    assertEq(p.shape.toString(), shape.toString());
  }
}

testShape();
