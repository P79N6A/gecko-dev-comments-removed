






assertEq(testLenientAndStrict('var eval;',
                              parsesSuccessfully,
                              parseRaisesException(SyntaxError)),
         true);
assertEq(testLenientAndStrict('var x,eval;',
                              parsesSuccessfully,
                              parseRaisesException(SyntaxError)),
         true);
assertEq(testLenientAndStrict('var arguments;',
                              parsesSuccessfully,
                              parseRaisesException(SyntaxError)),
         true);
assertEq(testLenientAndStrict('var x,arguments;',
                              parsesSuccessfully,
                              parseRaisesException(SyntaxError)),
         true);

reportCompare(true, true);
