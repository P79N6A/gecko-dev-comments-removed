











assertEq(testLenientAndStrict('let eval;',
                              parsesSuccessfully,
                              parseRaisesException(SyntaxError)),
         true);
assertEq(testLenientAndStrict('let x,eval;',
                              parsesSuccessfully,
                              parseRaisesException(SyntaxError)),
         true);
assertEq(testLenientAndStrict('let arguments;',
                              parsesSuccessfully,
                              parseRaisesException(SyntaxError)),
         true);
assertEq(testLenientAndStrict('let x,arguments;',
                              parsesSuccessfully,
                              parseRaisesException(SyntaxError)),
         true);
assertEq(testLenientAndStrict('const eval = undefined;',
                              parsesSuccessfully,
                              parseRaisesException(SyntaxError)),
         true);
assertEq(testLenientAndStrict('const x = undefined,eval = undefined;',
                              parsesSuccessfully,
                              parseRaisesException(SyntaxError)),
         true);
assertEq(testLenientAndStrict('const arguments = undefined;',
                              parsesSuccessfully,
                              parseRaisesException(SyntaxError)),
         true);
assertEq(testLenientAndStrict('const x = undefined,arguments = undefined;',
                              parsesSuccessfully,
                              parseRaisesException(SyntaxError)),
         true);





assertEq(testLenientAndStrict('for (let eval in [])break;',
                              parsesSuccessfully,
                              parseRaisesException(SyntaxError)),
         true);
assertEq(testLenientAndStrict('for (let [eval] in [])break;',
                              parsesSuccessfully,
                              parseRaisesException(SyntaxError)),
         true);
assertEq(testLenientAndStrict('for (let {x:eval} in [])break;',
                              parsesSuccessfully,
                              parseRaisesException(SyntaxError)),
         true);
assertEq(testLenientAndStrict('for (let arguments in [])break;',
                              parsesSuccessfully,
                              parseRaisesException(SyntaxError)),
         true);
assertEq(testLenientAndStrict('for (let [arguments] in [])break;',
                              parsesSuccessfully,
                              parseRaisesException(SyntaxError)),
         true);
assertEq(testLenientAndStrict('for (let {x:arguments} in [])break;',
                              parsesSuccessfully,
                              parseRaisesException(SyntaxError)),
         true);

reportCompare(true, true);
