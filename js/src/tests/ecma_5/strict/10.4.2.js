







assertEq(testLenientAndStrict("eval('010')",
                              completesNormally,
                              raisesException(SyntaxError)),
         true);





assertEq(testLenientAndStrict("eval('\"use strict\"; 010')",
                              raisesException(SyntaxError),
                              raisesException(SyntaxError)),
         true);


assertEq(testLenientAndStrict("var evil=eval; evil('010')",
                              completesNormally,
                              completesNormally),
         true);





assertEq(completesNormally("Function('010')"),
         true);
assertEq(raisesException(SyntaxError)("Function('\"use strict\"; 010')"),
         true);
