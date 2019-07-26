function testElement() {
  
  var a = [1,{},"a",false]
  var p = new ParallelArray(a);
  for (var i = 0; i < a.length; i++) {
    assertEq(p.get(i), p.get(i));
    assertEq(p.get(i), a[i]);
  }
  
  assertEq(p.get(42), undefined);
}

if (getBuildConfiguration().parallelJS)
  testElement();
