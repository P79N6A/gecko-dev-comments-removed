function testElement() {
  
  var p = new ParallelArray([2,2,2], function () { return 0; });
  assertEq(p[0].toString(), "<<0,0>,<0,0>>");
  
  assertEq(p[0] !== p[0], true);
  
  assertEq(p[42], undefined);
}

testElement();
