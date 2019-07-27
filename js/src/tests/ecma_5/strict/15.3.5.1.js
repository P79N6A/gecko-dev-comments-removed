






function fn() {
  return function(a, b, c) { };
}

assertEq(testLenientAndStrict('var f = fn(); f.length = 1; f.length',
                              returns(3), raisesException(TypeError)),
         true);
assertEq(testLenientAndStrict('var f = fn(); delete f.length',
                              returns(false), raisesException(TypeError)),
         true);

reportCompare(true, true);
