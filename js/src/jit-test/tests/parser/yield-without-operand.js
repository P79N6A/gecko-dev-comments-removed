

load(libdir + "asserts.js");

assertNoWarning(() => Function("yield"), SyntaxError,
                "yield followed by EOF is fine");
assertNoWarning(() => Function("yield;"), SyntaxError,
                "yield followed by semicolon is fine");
assertNoWarning(() => Function("yield\n  print('ok');"), SyntaxError,
                "yield followed by newline is fine");

assertNoWarning(() => eval("(function () { yield; })"), SyntaxError,
                "yield followed by semicolon in eval code is fine");
assertNoWarning(() => eval("(function () { yield })"), SyntaxError,
                "yield followed by } in eval code is fine");

assertNoWarning(() => Function("yield 0;"),
                "yield with an operand should be fine");
assertNoWarning(() => Function("yield 0"),
                "yield with an operand should be fine, even without a semicolon");
