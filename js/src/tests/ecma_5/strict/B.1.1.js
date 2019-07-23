







assertEq(testLenientAndStrict('010',
                              parsesSuccessfully,
                              parseRaisesException(SyntaxError)),
         true);


assertEq(parseRaisesException(SyntaxError)
         ('function f() { "use strict"; 010; }'),
         true);
                              





assertEq(parsesSuccessfully('function f() { "use strict"; }; 010'),
         true);


assertEq(parsesSuccessfully('function f() { 010; }'),
         true);

reportCompare(true, true);
