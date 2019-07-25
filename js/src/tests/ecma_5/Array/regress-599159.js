




var b = Object.create(Array.prototype);
b.length = 12;
assertEq(b.length, 12);

reportCompare(true,true);
