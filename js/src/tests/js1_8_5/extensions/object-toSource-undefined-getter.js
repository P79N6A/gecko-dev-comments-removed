


var desc = { get: undefined, set: undefined, configurable: true, enumerable: true };
var obj = Object.defineProperty({}, "prop", desc);
assertEq(obj.toSource(), "({})");

if (typeof reportCompare === "function")
    reportCompare(0, 0);
