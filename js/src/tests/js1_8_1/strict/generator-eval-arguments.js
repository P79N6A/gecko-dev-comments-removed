










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
                              parseRaisesException(SyntaxError),
                              parseRaisesException(SyntaxError)),
         true);
assertEq(testLenientAndStrict('(1 for ([arguments] in []))',
                              parseRaisesException(SyntaxError),
                              parseRaisesException(SyntaxError)),
         true);
assertEq(testLenientAndStrict('(1 for ({x:arguments} in []))',
                              parseRaisesException(SyntaxError),
                              parseRaisesException(SyntaxError)),
         true);

reportCompare(true, true);
