function testElement() {
  
  ParallelArray.prototype[42] = "foo";
  var p = new ParallelArray([1,2,3,4]);
  assertEq(p[42], "foo");
}

testElement();
