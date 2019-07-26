



var desc = Object.getOwnPropertyDescriptor(Array, "from");
assertEq(desc.configurable, true);
assertEq(desc.enumerable, false);
assertEq(desc.writable, true);
assertEq(Array.from.length, 1);
assertThrowsInstanceOf(() => new Array.from(), TypeError);  

if (typeof reportCompare === 'function')
    reportCompare(0, 0);
