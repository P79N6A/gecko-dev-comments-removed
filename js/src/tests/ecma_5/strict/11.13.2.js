










assertEq(testLenientAndStrict('arguments+=1',
                              parsesSuccessfully,
                              parseRaisesException(SyntaxError)),
         true);
assertEq(testLenientAndStrict('eval+=1',
                              parsesSuccessfully,
                              parseRaisesException(SyntaxError)),
         true);
assertEq(testLenientAndStrict('(arguments)+=1',
                              parsesSuccessfully,
                              parseRaisesException(SyntaxError)),
         true);
assertEq(testLenientAndStrict('(eval)+=1',
                              parsesSuccessfully,
                              parseRaisesException(SyntaxError)),
         true);

reportCompare(true, true);
