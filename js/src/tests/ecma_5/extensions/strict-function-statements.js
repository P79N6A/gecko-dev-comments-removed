





assertEq(testLenientAndStrict("function f() { }",
                              parsesSuccessfully,
                              parsesSuccessfully),
         true);


assertEq(testLenientAndStrict("{ function f() { } }",
                              parsesSuccessfully,
                              parseRaisesException(SyntaxError)),
         true);


assertEq(testLenientAndStrict("{ (function f() { }) }",
                              parsesSuccessfully,
                              parsesSuccessfully),
         true);


assertEq(testLenientAndStrict("if (true) function f() { }",
                              parsesSuccessfully,
                              parseRaisesException(SyntaxError)),
         true);
assertEq(testLenientAndStrict("while (true) function f() { }",
                              parsesSuccessfully,
                              parseRaisesException(SyntaxError)),
         true);
assertEq(testLenientAndStrict("do function f() { } while (true);",
                              parsesSuccessfully,
                              parseRaisesException(SyntaxError)),
         true);
assertEq(testLenientAndStrict("for(;;) function f() { }",
                              parsesSuccessfully,
                              parseRaisesException(SyntaxError)),
         true);
assertEq(testLenientAndStrict("for(x in []) function f() { }",
                              parsesSuccessfully,
                              parseRaisesException(SyntaxError)),
         true);
assertEq(testLenientAndStrict("with(o) function f() { }",
                              parsesSuccessfully,
                              parseRaisesException(SyntaxError)),
         true);
assertEq(testLenientAndStrict("switch(1) { case 1: function f() { } }",
                              parsesSuccessfully,
                              parseRaisesException(SyntaxError)),
         true);
assertEq(testLenientAndStrict("x: function f() { }",
                              parsesSuccessfully,
                              parseRaisesException(SyntaxError)),
         true);
assertEq(testLenientAndStrict("try { function f() { } } catch (x) { }",
                              parsesSuccessfully,
                              parseRaisesException(SyntaxError)),
         true);


assertEq(testLenientAndStrict("if (true) (function f() { })",
                              parsesSuccessfully,
                              parsesSuccessfully),
         true);


assertEq(parsesSuccessfully("function f() { function g() { } }"),
         true);


assertEq(parsesSuccessfully("function f() { if (true) function g() { } }"),
         true);

assertEq(parseRaisesException(SyntaxError)
         ("function f() { 'use strict'; if (true) function g() { } }"),
         true);

assertEq(parseRaisesException(SyntaxError)
         ("function f() { 'use strict'; { function g() { } } }"),
         true);

assertEq(parsesSuccessfully("function f() { 'use strict'; if (true) (function g() { }) }"),
         true);

assertEq(parsesSuccessfully("function f() { 'use strict'; { (function g() { }) } }"),
         true);


assertEq(testLenientAndStrict("function f() { }",
                              completesNormally,
                              completesNormally),
         true);
assertEq(testLenientAndStrict("{ function f() { } }",
                              completesNormally,
                              raisesException(SyntaxError)),
         true);

reportCompare(true, true);
