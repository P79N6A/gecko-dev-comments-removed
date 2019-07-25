






function fn() {
  return function(a, b, c) { };
}

assertEq(testLenientAndStrict('var f = fn(); delete f.prototype',
                              returns(false), raisesException(TypeError)),
         true);

reportCompare(true, true);
