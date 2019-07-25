function testHoles() {
  function f1(a) { return a * 42; }
  function f2(a,b) { return a * b; }
  
  
  
  
  
  var p = new ParallelArray([,1]);
  assertEq(p[0] * 42, NaN);
  var m = p.map(f1);
  assertEq(m[0], NaN);
  assertEq(m[1], 42);
  var r = p.reduce(f2);
  assertEq(r, NaN);
  var s = p.scan(f2);
  assertEq(s[0] * 42, NaN);
  assertEq(s[1], NaN);
  var k = p.scatter([1,0]);
  assertEq(k[0], 1);
  assertEq(k[1] * 42, NaN);
  var l = p.filter([1,0]);
  assertEq(l[0] * 42, NaN);
  var p2 = p.partition(1);
  assertEq(p2[0][0] * 42, NaN);
  var g = p.get([0]);
  assertEq(g * 42, NaN);
}

testHoles();
