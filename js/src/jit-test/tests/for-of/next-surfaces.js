

assertEq(Iterator.prototype.next.length, 0);
var desc = Object.getOwnPropertyDescriptor(Iterator.prototype, "next");
assertEq(desc.configurable, true);
assertEq(desc.enumerable, false);
assertEq(desc.writable, true);
