






function str() {
  return new String("foo");
}

assertEq(testLenientAndStrict('var s = str(); s.length = 1; s.length',
                              returns(3), raisesException(TypeError)),
         true);
assertEq(testLenientAndStrict('var s = str(); delete s.length',
                              returns(false), raisesException(TypeError)),
         true);

assertEq(testLenientAndStrict('"foo".length = 1',
                              returns(1), raisesException(TypeError)),
         true);
assertEq(testLenientAndStrict('delete "foo".length',
                              returns(false), raisesException(TypeError)),
         true);

reportCompare(true, true);
