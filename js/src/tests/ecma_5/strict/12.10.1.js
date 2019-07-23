










assertEq(testLenientAndStrict('with (1) {}',
                              completesNormally,
                              raisesException(SyntaxError)),
         true);


assertEq(testLenientAndStrict('function f() { "use strict"; with (1) {} }',
                              parseRaisesException(SyntaxError),
                              parseRaisesException(SyntaxError)),
         true);
                              




assertEq(parsesSuccessfully('function f() { "use strict"; }; with (1) {}'),
         true);
