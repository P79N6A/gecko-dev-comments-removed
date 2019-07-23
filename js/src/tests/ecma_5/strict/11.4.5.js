










assertEq(testLenientAndStrict('--arguments',
                              parsesSuccessfully,
                              parseRaisesException(SyntaxError)),
         true);
assertEq(testLenientAndStrict('--eval',
                              parsesSuccessfully,
                              parseRaisesException(SyntaxError)),
         true);
assertEq(testLenientAndStrict('--(arguments)',
                              parsesSuccessfully,
                              parseRaisesException(SyntaxError)),
         true);
assertEq(testLenientAndStrict('--(eval)',
                              parsesSuccessfully,
                              parseRaisesException(SyntaxError)),
         true);

reportCompare(true, true);
