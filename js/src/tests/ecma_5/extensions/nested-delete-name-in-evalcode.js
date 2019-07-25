



var BUGNUMBER = 616294;
var summary =
  "|delete x| inside a function in eval code, where that eval code includes " +
  "|var x| at top level, actually does delete the binding for x";

print(BUGNUMBER + ": " + summary);





var f;

function testOuterLet()
{
  return eval("let x; (function() { return delete x; })");
}

f = testOuterLet();

assertEq(f(), true); 
assertEq(f(), true); 


function testOuterForLet()
{
  return eval("for (let x; false; ); (function() { return delete x; })");
}

f = testOuterForLet();

assertEq(f(), true); 


function testOuterForInLet()
{
  return eval("for (let x in {}); (function() { return delete x; })");
}

f = testOuterForInLet();

assertEq(f(), true); 
assertEq(f(), true); 


function testOuterNestedVarInLetBlock()
{
  return eval("let (x = 7) { var x = 9; } (function() { return delete x; })");
}

f = testOuterNestedVarInLetBlock();

assertEq(f(), true); 
assertEq(f(), true); 
assertEq(f(), true); 


function testOuterNestedVarInForLet()
{
  return eval("for (let q = 0; q < 5; q++) { var x; } (function() { return delete x; })");
}

f = testOuterNestedVarInForLet();

assertEq(f(), true); 
assertEq(f(), true); 


function testArgumentShadowLet()
{
  return eval("let x; (function(x) { return delete x; })");
}

f = testArgumentShadowLet();

assertEq(f(), false); 


function testFunctionLocal()
{
  return eval("(function() { let x; return delete x; })");
}

f = testFunctionLocal();

assertEq(f(), false); 




if (typeof reportCompare === "function")
  reportCompare(true, true);

print("All tests passed!");
