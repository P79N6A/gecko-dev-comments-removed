








var BUGNUMBER = 1046964;
var summary =
  "Misplaced directives (e.g. 'use strict') should trigger warnings if they " +
  "contradict the actually-used semantics";

print(BUGNUMBER + ": " + summary);





options("strict");
options("werror");

function evaluateNoRval(code)
{
  evaluate(code, { compileAndGo: true, isRunOnce: true, noScriptRval: true });
}

function expectSyntaxError(code)
{
  try
  {
    evaluateNoRval(code);
    throw new Error("didn't throw");
  }
  catch (e)
  {
    assertEq(e instanceof SyntaxError, true,
             "should have thrown a SyntaxError, instead got:\n" +
             "    " + e + "\n" +
             "when evaluating:\n" +
             "    " + code);
  }
}

expectSyntaxError("function f1() {} 'use strict'; function f2() {}");
expectSyntaxError("function f3() { var x; 'use strict'; }");

if (isAsmJSCompilationAvailable())
  expectSyntaxError("function f4() {} 'use asm'; function f5() {}");
expectSyntaxError("function f6() { var x; 'use strict'; }");
if (isAsmJSCompilationAvailable())
  expectSyntaxError("'use asm'; function f7() {}");



evaluateNoRval("'use strict'; function f8() {} 'use strict'; function f9() {}");
evaluateNoRval("'use strict'; function f10() { var z; 'use strict' }");

if (isAsmJSCompilationAvailable())
  evaluateNoRval("function f11() { 'use asm'; return {}; }");



if (typeof reportCompare === "function")
  reportCompare(true, true);

print("Tests complete");
