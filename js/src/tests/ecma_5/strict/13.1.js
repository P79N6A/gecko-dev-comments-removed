















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















assertEq(testLenientAndStrict('function f(eval){}',
                              parsesSuccessfully,
                              parseRaisesException(SyntaxError)),
         true);
assertEq(testLenientAndStrict('function f([eval]){}',
                              parsesSuccessfully,
                              parseRaisesException(SyntaxError)),
         true);
assertEq(testLenientAndStrict('function f({x:eval}){}',
                              parsesSuccessfully,
                              parseRaisesException(SyntaxError)),
         true);
assertEq(testLenientAndStrict('function eval(){}',
                              parsesSuccessfully,
                              parseRaisesException(SyntaxError)),
         true);
assertEq(testLenientAndStrict('function f(eval){"use strict";}',
                              parseRaisesException(SyntaxError),
                              parseRaisesException(SyntaxError)),
         true);
assertEq(testLenientAndStrict('function f([eval]){"use strict";}',
                              parseRaisesException(SyntaxError),
                              parseRaisesException(SyntaxError)),
         true);
assertEq(testLenientAndStrict('function f({x:eval}){"use strict";}',
                              parseRaisesException(SyntaxError),
                              parseRaisesException(SyntaxError)),
         true);
assertEq(testLenientAndStrict('function eval(){"use strict";}',
                              parseRaisesException(SyntaxError),
                              parseRaisesException(SyntaxError)),
         true);
assertEq(testLenientAndStrict('(function f(eval){})',
                              parsesSuccessfully,
                              parseRaisesException(SyntaxError)),
         true);
assertEq(testLenientAndStrict('(function f([eval]){})',
                              parsesSuccessfully,
                              parseRaisesException(SyntaxError)),
         true);
assertEq(testLenientAndStrict('(function f({x:eval}){})',
                              parsesSuccessfully,
                              parseRaisesException(SyntaxError)),
         true);
assertEq(testLenientAndStrict('(function eval(){})',
                              parsesSuccessfully,
                              parseRaisesException(SyntaxError)),
         true);
assertEq(testLenientAndStrict('(function f(eval){"use strict";})',
                              parseRaisesException(SyntaxError),
                              parseRaisesException(SyntaxError)),
         true);
assertEq(testLenientAndStrict('(function f([eval]){"use strict";})',
                              parseRaisesException(SyntaxError),
                              parseRaisesException(SyntaxError)),
         true);
assertEq(testLenientAndStrict('(function f({x:eval}){"use strict";})',
                              parseRaisesException(SyntaxError),
                              parseRaisesException(SyntaxError)),
         true);
assertEq(testLenientAndStrict('(function eval(){"use strict";})',
                              parseRaisesException(SyntaxError),
                              parseRaisesException(SyntaxError)),
         true);
assertEq(testLenientAndStrict('(function f(eval) 2)',
                              parsesSuccessfully,
                              parseRaisesException(SyntaxError)),
         true);
assertEq(testLenientAndStrict('(function f([eval]) 2)',
                              parsesSuccessfully,
                              parseRaisesException(SyntaxError)),
         true);
assertEq(testLenientAndStrict('(function f({x:eval}) 2)',
                              parsesSuccessfully,
                              parseRaisesException(SyntaxError)),
         true);
assertEq(testLenientAndStrict('(function eval() 2)',
                              parsesSuccessfully,
                              parseRaisesException(SyntaxError)),
         true);
assertEq(testLenientAndStrict('({get x(eval){}})',
                              parsesSuccessfully,
                              parseRaisesException(SyntaxError)),
         true);
assertEq(testLenientAndStrict('({get x([eval]){}})',
                              parsesSuccessfully,
                              parseRaisesException(SyntaxError)),
         true);
assertEq(testLenientAndStrict('({get x({x:eval}){}})',
                              parsesSuccessfully,
                              parseRaisesException(SyntaxError)),
         true);
assertEq(testLenientAndStrict('({get x(eval){"use strict";}})',
                              parseRaisesException(SyntaxError),
                              parseRaisesException(SyntaxError)),
         true);
assertEq(testLenientAndStrict('({get x([eval]){"use strict";}})',
                              parseRaisesException(SyntaxError),
                              parseRaisesException(SyntaxError)),
         true);
assertEq(testLenientAndStrict('({get x({x:eval}){"use strict";}})',
                              parseRaisesException(SyntaxError),
                              parseRaisesException(SyntaxError)),
         true);
assertEq(testLenientAndStrict('({set x(eval){}})',
                              parsesSuccessfully,
                              parseRaisesException(SyntaxError)),
         true);
assertEq(testLenientAndStrict('({set x([eval]){}})',
                              parsesSuccessfully,
                              parseRaisesException(SyntaxError)),
         true);
assertEq(testLenientAndStrict('({set x({x:eval}){}})',
                              parsesSuccessfully,
                              parseRaisesException(SyntaxError)),
         true);
assertEq(testLenientAndStrict('({set x(eval){"use strict";}})',
                              parseRaisesException(SyntaxError),
                              parseRaisesException(SyntaxError)),
         true);
assertEq(testLenientAndStrict('({set x([eval]){"use strict";}})',
                              parseRaisesException(SyntaxError),
                              parseRaisesException(SyntaxError)),
         true);
assertEq(testLenientAndStrict('({set x({x:eval}){"use strict";}})',
                              parseRaisesException(SyntaxError),
                              parseRaisesException(SyntaxError)),
         true);
assertEq(testLenientAndStrict('function f(arguments){}',
                              parsesSuccessfully,
                              parseRaisesException(SyntaxError)),
         true);
assertEq(testLenientAndStrict('function f([arguments]){}',
                              parsesSuccessfully,
                              parseRaisesException(SyntaxError)),
         true);
assertEq(testLenientAndStrict('function f({x:arguments}){}',
                              parsesSuccessfully,
                              parseRaisesException(SyntaxError)),
         true);
assertEq(testLenientAndStrict('function arguments(){}',
                              parsesSuccessfully,
                              parseRaisesException(SyntaxError)),
         true);
assertEq(testLenientAndStrict('function f(arguments){"use strict";}',
                              parseRaisesException(SyntaxError),
                              parseRaisesException(SyntaxError)),
         true);
assertEq(testLenientAndStrict('function f([arguments]){"use strict";}',
                              parseRaisesException(SyntaxError),
                              parseRaisesException(SyntaxError)),
         true);
assertEq(testLenientAndStrict('function f({x:arguments}){"use strict";}',
                              parseRaisesException(SyntaxError),
                              parseRaisesException(SyntaxError)),
         true);
assertEq(testLenientAndStrict('function arguments(){"use strict";}',
                              parseRaisesException(SyntaxError),
                              parseRaisesException(SyntaxError)),
         true);
assertEq(testLenientAndStrict('(function f(arguments){})',
                              parsesSuccessfully,
                              parseRaisesException(SyntaxError)),
         true);
assertEq(testLenientAndStrict('(function f([arguments]){})',
                              parsesSuccessfully,
                              parseRaisesException(SyntaxError)),
         true);
assertEq(testLenientAndStrict('(function f({x:arguments}){})',
                              parsesSuccessfully,
                              parseRaisesException(SyntaxError)),
         true);
assertEq(testLenientAndStrict('(function arguments(){})',
                              parsesSuccessfully,
                              parseRaisesException(SyntaxError)),
         true);
assertEq(testLenientAndStrict('(function f(arguments){"use strict";})',
                              parseRaisesException(SyntaxError),
                              parseRaisesException(SyntaxError)),
         true);
assertEq(testLenientAndStrict('(function f([arguments]){"use strict";})',
                              parseRaisesException(SyntaxError),
                              parseRaisesException(SyntaxError)),
         true);
assertEq(testLenientAndStrict('(function f({x:arguments}){"use strict";})',
                              parseRaisesException(SyntaxError),
                              parseRaisesException(SyntaxError)),
         true);
assertEq(testLenientAndStrict('(function arguments(){"use strict";})',
                              parseRaisesException(SyntaxError),
                              parseRaisesException(SyntaxError)),
         true);
assertEq(testLenientAndStrict('(function f(arguments) 2)',
                              parsesSuccessfully,
                              parseRaisesException(SyntaxError)),
         true);
assertEq(testLenientAndStrict('(function f([arguments]) 2)',
                              parsesSuccessfully,
                              parseRaisesException(SyntaxError)),
         true);
assertEq(testLenientAndStrict('(function f({x:arguments}) 2)',
                              parsesSuccessfully,
                              parseRaisesException(SyntaxError)),
         true);
assertEq(testLenientAndStrict('(function arguments() 2)',
                              parsesSuccessfully,
                              parseRaisesException(SyntaxError)),
         true);
assertEq(testLenientAndStrict('({get x(arguments){}})',
                              parsesSuccessfully,
                              parseRaisesException(SyntaxError)),
         true);
assertEq(testLenientAndStrict('({get x([arguments]){}})',
                              parsesSuccessfully,
                              parseRaisesException(SyntaxError)),
         true);
assertEq(testLenientAndStrict('({get x({x:arguments}){}})',
                              parsesSuccessfully,
                              parseRaisesException(SyntaxError)),
         true);
assertEq(testLenientAndStrict('({get x(arguments){"use strict";}})',
                              parseRaisesException(SyntaxError),
                              parseRaisesException(SyntaxError)),
         true);
assertEq(testLenientAndStrict('({get x([arguments]){"use strict";}})',
                              parseRaisesException(SyntaxError),
                              parseRaisesException(SyntaxError)),
         true);
assertEq(testLenientAndStrict('({get x({x:arguments}){"use strict";}})',
                              parseRaisesException(SyntaxError),
                              parseRaisesException(SyntaxError)),
         true);
assertEq(testLenientAndStrict('({set x(arguments){}})',
                              parsesSuccessfully,
                              parseRaisesException(SyntaxError)),
         true);
assertEq(testLenientAndStrict('({set x([arguments]){}})',
                              parsesSuccessfully,
                              parseRaisesException(SyntaxError)),
         true);
assertEq(testLenientAndStrict('({set x({x:arguments}){}})',
                              parsesSuccessfully,
                              parseRaisesException(SyntaxError)),
         true);
assertEq(testLenientAndStrict('({set x(arguments){"use strict";}})',
                              parseRaisesException(SyntaxError),
                              parseRaisesException(SyntaxError)),
         true);
assertEq(testLenientAndStrict('({set x([arguments]){"use strict";}})',
                              parseRaisesException(SyntaxError),
                              parseRaisesException(SyntaxError)),
         true);
assertEq(testLenientAndStrict('({set x({x:arguments}){"use strict";}})',
                              parseRaisesException(SyntaxError),
                              parseRaisesException(SyntaxError)),
         true);







assertEq(testLenientAndStrict('Function("eval","")',
                              completesNormally,
                              completesNormally),
         true);
assertEq(testLenientAndStrict('Function("eval","\'use strict\';")',
                              raisesException(SyntaxError),
                              raisesException(SyntaxError)),
         true);
assertEq(testLenientAndStrict('Function("arguments","")',
                              completesNormally,
                              completesNormally),
         true);
assertEq(testLenientAndStrict('Function("arguments","\'use strict\';")',
                              raisesException(SyntaxError),
                              raisesException(SyntaxError)),
         true);

