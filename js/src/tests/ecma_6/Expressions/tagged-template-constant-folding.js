



var BUGNUMBER = 1182373;
var summary =
  "Don't let constant-folding in the MemberExpression part of a tagged " +
  "template cause an incorrect |this| be passed to the callee";

print(BUGNUMBER + ": " + summary);





var prop = "global";

var obj = { prop: "obj", f: function() { return this.prop; } };

assertEq(obj.f``, "obj");
assertEq((true ? obj.f : null)``, "global");



if (typeof reportCompare === "function")
  reportCompare(true, true);

print("Tests complete");
