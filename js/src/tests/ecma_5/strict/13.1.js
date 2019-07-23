















assertEq(testLenientAndStrict('function(x,y) {}',
                              parsesSuccessfully,
                              parsesSuccessfully),
         true);
assertEq(testLenientAndStrict('function(x,x) {}',
                              parsesSuccessfully,
                              parseRaisesException(SyntaxError)),
         true);
assertEq(testLenientAndStrict('function(x,y,z,y) {}',
                              parsesSuccessfully,
                              parseRaisesException(SyntaxError)),
         true);


assertEq(testLenientAndStrict('function(a,b,c,d,e,f,g,h,d) {}',
                              parsesSuccessfully,
                              parseRaisesException(SyntaxError)),
         true);





assertEq(testLenientAndStrict('function([x,y]) {}',
                              parsesSuccessfully,
                              parsesSuccessfully),
         true);
assertEq(testLenientAndStrict('function([x,x]){}',
                              parseRaisesException(SyntaxError),
                              parseRaisesException(SyntaxError)),
         true);
assertEq(testLenientAndStrict('function(x,[x]){}',
                              parseRaisesException(SyntaxError),
                              parseRaisesException(SyntaxError)),
         true);





assertEq(testLenientAndStrict('function(x,x) { "use strict" };',
                              parseRaisesException(SyntaxError),
                              parseRaisesException(SyntaxError)),
         true);






assertEq(testLenientAndStrict('Function("x","x","")',
                              completesNormally,
                              completesNormally),
         true);
assertEq(testLenientAndStrict('Function("x","y","")',
                              completesNormally,
                              completesNormally),
         true);
assertEq(testLenientAndStrict('Function("x","x","\'use strict\'")',
                              raisesException(SyntaxError),
                              raisesException(SyntaxError)),
         true);
assertEq(testLenientAndStrict('Function("x","y","\'use strict\'")',
                              completesNormally,
                              completesNormally),
         true);






assertEq(testLenientAndStrict('({get x(y,y) {}})',
                               parsesSuccessfully,
                               parseRaisesException(SyntaxError)),
         true);
assertEq(testLenientAndStrict('({get x(y,y) { "use strict"; }})',
                              parseRaisesException(SyntaxError),
                              parseRaisesException(SyntaxError)),
         true);
assertEq(testLenientAndStrict('({set x(y,y) {}})',
                               parsesSuccessfully,
                               parseRaisesException(SyntaxError)),
         true);
assertEq(testLenientAndStrict('({set x(y,y) { "use strict"; }})',
                              parseRaisesException(SyntaxError),
                              parseRaisesException(SyntaxError)),
         true);





assertEq(testLenientAndStrict('(function (x,x) 2)',
                               parsesSuccessfully,
                               parseRaisesException(SyntaxError)),
         true);
assertEq(testLenientAndStrict('(function (x,y) 2)',
                              parsesSuccessfully,
                              parsesSuccessfully),
         true);
