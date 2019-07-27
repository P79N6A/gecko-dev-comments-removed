






function fn() {
  return function(a, b, c) { };
}

assertEq(testLenientAndStrict('var f = fn(); f.length = 1; f.length',
                              returns(3), raisesException(TypeError)),
         true);

reportCompare(true, true);
