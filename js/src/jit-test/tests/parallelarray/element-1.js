function testElement() {
  
  var a = [1,{},"a",false]
  var p = new ParallelArray(a);
  for (var i = 0; i < a.length; i++) {
    assertEq(p[i], p[i]);
    assertEq(p[i], a[i]);
  }
  
  assertEq(p[42], undefined);
}

testElement();
