




var gTestfile = 'isPrototypeOf.js';
var BUGNUMBER = 619283;
var summary = "Object.prototype.isPrototypeOf";

print(BUGNUMBER + ": " + summary);





function expectThrowTypeError(fun)
{
  try
  {
    var r = fun();
    throw new Error("didn't throw TypeError, returned " + r);
  }
  catch (e)
  {
    assertEq(e instanceof TypeError, true,
             "didn't throw TypeError, got: " + e);
  }
}

var isPrototypeOf = Object.prototype.isPrototypeOf;




assertEq(isPrototypeOf(), false);
assertEq(isPrototypeOf(1), false);
assertEq(isPrototypeOf(Number.MAX_VALUE), false);
assertEq(isPrototypeOf(NaN), false);
assertEq(isPrototypeOf(""), false);
assertEq(isPrototypeOf("sesquicentennial"), false);
assertEq(isPrototypeOf(true), false);
assertEq(isPrototypeOf(false), false);
assertEq(isPrototypeOf(0.72), false);
assertEq(isPrototypeOf(undefined), false);
assertEq(isPrototypeOf(null), false);






var protoGlobal = Object.create(this);
expectThrowTypeError(function() { isPrototypeOf.call(null, {}); });
expectThrowTypeError(function() { isPrototypeOf.call(undefined, {}); });
expectThrowTypeError(function() { isPrototypeOf({}); });
expectThrowTypeError(function() { isPrototypeOf.call(null, protoGlobal); });
expectThrowTypeError(function() { isPrototypeOf.call(undefined, protoGlobal); });
expectThrowTypeError(function() { isPrototypeOf(protoGlobal); });










assertEq(Object.prototype.isPrototypeOf(Object.prototype), false);
assertEq(String.prototype.isPrototypeOf({}), false);
assertEq(Object.prototype.isPrototypeOf(Object.create(null)), false);


assertEq(Object.prototype.isPrototypeOf({}), true);
assertEq(this.isPrototypeOf(protoGlobal), true);
assertEq(Object.prototype.isPrototypeOf({}), true);
assertEq(Object.prototype.isPrototypeOf(new Number(17)), true);
assertEq(Object.prototype.isPrototypeOf(function(){}), true);




if (typeof reportCompare === "function")
  reportCompare(true, true);

print("All tests passed!");
