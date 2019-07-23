














assertEq(testLenientAndStrict('undeclared=1',
                              completesNormally,
                              raisesException(ReferenceError)),
         true);





assertEq(testLenientAndStrict('var var_declared; var_declared=1',
                              completesNormally,
                              completesNormally),
         true);





assertEq(testLenientAndStrict('undeclared_at_compiletime=1',
                              parsesSuccessfully,
                              parsesSuccessfully),
         true);
