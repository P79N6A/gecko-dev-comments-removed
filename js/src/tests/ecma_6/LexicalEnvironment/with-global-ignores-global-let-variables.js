



let v = "global-v";

function f(v, global)
{
  with (global)
    return v;
}

assertEq(f("argument-v", this), "argument-v");

if (typeof reportCompare === "function")
  reportCompare(true, true);

print("Tests complete");
