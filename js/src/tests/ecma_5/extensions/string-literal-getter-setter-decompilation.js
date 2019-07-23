


var f;
try
{
  f = eval("(function literalInside() { return { set 'c d e'(v) { } }; })");
}
catch (e)
{
  assertEq(true, false,
           "string-literal property name in setter in object literal in " +
           "function statement failed to parse: " + e);
}

var fstr = "" + f;
assertEq(fstr.indexOf("set") < fstr.indexOf("c d e"), true,
         "should be using new-style syntax with string literal in place of " +
         "property identifier");
assertEq(fstr.indexOf("setter") < 0, true, "using old-style syntax?");

var o = f();
var ostr = "" + o;
assertEq("c d e" in o, true, "missing the property?");
assertEq("set" in Object.getOwnPropertyDescriptor(o, "c d e"), true,
         "'c d e' property not a setter?");







reportCompare(true, true);
