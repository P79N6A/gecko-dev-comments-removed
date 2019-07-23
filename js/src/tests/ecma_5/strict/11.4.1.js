







assertEq(testLenientAndStrict('delete x;',
                              parsesSuccessfully,
                              parseRaisesException(SyntaxError)),
         true);





assertEq(testLenientAndStrict('delete (x);',
                              parsesSuccessfully,
                              parseRaisesException(SyntaxError)),
         true);


assertEq(testLenientAndStrict('delete x.y;',
                              parsesSuccessfully,
                              parsesSuccessfully),
         true);


assertEq(testLenientAndStrict('function f() { delete x; }',
                              parsesSuccessfully,
                              parseRaisesException(SyntaxError)),
         true);


assertEq(testLenientAndStrict('function f() { "use strict"; delete x; }',
                              parseRaisesException(SyntaxError),
                              parseRaisesException(SyntaxError)),
         true);

reportCompare(true, true);
