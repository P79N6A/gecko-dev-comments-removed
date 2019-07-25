














assertEq(testLenientAndStrict('undeclared=1',
                              completesNormally,
                              raisesException(ReferenceError)),
         true);





assertEq(testLenientAndStrict('var var_declared; var_declared=1',
                              completesNormally,
                              completesNormally),
         true);





assertEq(testLenientAndStrict('undeclared_at_compiletime=1',
                              parsesSuccessfully,
                              parsesSuccessfully),
         true);

function obj() {
  var o = { x: 1, y: 1 };
  Object.defineProperty(o, 'x', { writable: false });
  return o;
}


function in_strict_with(expr) {
  return "with(obj()) { (function () { 'use strict'; " + expr + " })(); }";
}

assertEq(raisesException(TypeError)(in_strict_with('x = 2; y = 2;')), true);
assertEq(raisesException(TypeError)(in_strict_with('x++;')), true);
assertEq(raisesException(TypeError)(in_strict_with('++x;')), true);
assertEq(raisesException(TypeError)(in_strict_with('x--;')), true);
assertEq(raisesException(TypeError)(in_strict_with('--x;')), true);

reportCompare(true, true);
