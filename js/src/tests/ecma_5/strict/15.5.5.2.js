






assertEq(testLenientAndStrict('"foo"[0] = 1',
                              returns(1), raisesException(TypeError)),
         true);
assertEq(testLenientAndStrict('delete "foo"[0]',
                              returns(false), raisesException(TypeError)),
         true);

reportCompare(true, true);
