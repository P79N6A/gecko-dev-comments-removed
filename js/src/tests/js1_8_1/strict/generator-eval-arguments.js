










assertEq(testLenientAndStrict('(1 for (eval in []))',
                              parsesSuccessfully,
                              parseRaisesException(SyntaxError)),
         true);
assertEq(testLenientAndStrict('(1 for ([eval] in []))',
                              parsesSuccessfully,
                              parseRaisesException(SyntaxError)),
         true);
assertEq(testLenientAndStrict('(1 for ({x:eval} in []))',
                              parsesSuccessfully,
                              parseRaisesException(SyntaxError)),
         true);
assertEq(testLenientAndStrict('(1 for (arguments in []))',
                              parsesSuccessfully,
                              parseRaisesException(SyntaxError)),
         true);
assertEq(testLenientAndStrict('(1 for ([arguments] in []))',
                              parsesSuccessfully,
                              parseRaisesException(SyntaxError)),
         true);
assertEq(testLenientAndStrict('(1 for ({x:arguments} in []))',
                              parsesSuccessfully,
                              parseRaisesException(SyntaxError)),
         true);

reportCompare(true, true);
