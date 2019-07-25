






function fn() {
  return function f(a, b, c) { };
}

assertEq(testLenientAndStrict('var f = fn(); f.name = "g"; f.name',
                              returns("f"), raisesException(TypeError)),
         true);
assertEq(testLenientAndStrict('var f = fn(); delete f.name',
                              returns(false), raisesException(TypeError)),
         true);

assertEq(testLenientAndStrict('var f = fn(); f.arity = 4; f.arity',
                              returns(3), raisesException(TypeError)),
         true);
assertEq(testLenientAndStrict('var f = fn(); delete f.arity',
                              returns(false), raisesException(TypeError)),
         true);

reportCompare(true, true);
