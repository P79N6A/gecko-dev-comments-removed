




var desc = Object.getOwnPropertyDescriptor(this, "Symbol");
assertEq(desc.configurable, true);
assertEq(desc.enumerable, false);
assertEq(desc.writable, true);
assertEq(typeof Symbol, "function");
assertEq(Symbol.length, 1);

desc = Object.getOwnPropertyDescriptor(Symbol, "prototype");
assertEq(desc.configurable, false);
assertEq(desc.enumerable, false);
assertEq(desc.writable, false);

assertEq(Symbol.prototype.constructor, Symbol);
desc = Object.getOwnPropertyDescriptor(Symbol.prototype, "constructor");
assertEq(desc.configurable, true);
assertEq(desc.enumerable, false);
assertEq(desc.writable, true);

if (typeof reportCompare === "function")
    reportCompare(0, 0);
