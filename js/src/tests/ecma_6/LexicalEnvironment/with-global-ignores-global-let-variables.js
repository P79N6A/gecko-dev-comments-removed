



let v = "global-v";

function f(v, global)
{
  with (global)
    return v;
}



var AssertEq = typeof reportCompare === "function"
             ? (act, exp, msg) => reportCompare(exp, act, msg)
             : assertEq;

AssertEq(f("argument-v", this), "argument-v",
         "let-var shouldn't appear in global for |with| purposes");

if (typeof reportCompare === "function")
  reportCompare(true, true);

print("Tests complete");
