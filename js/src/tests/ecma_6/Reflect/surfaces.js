




assertEq(typeof Reflect, 'object');
assertEq(Object.getPrototypeOf(Reflect), Object.prototype);
assertEq(Reflect.toString(), '[object Object]');

var desc = Object.getOwnPropertyDescriptor(this, "Reflect");
assertEq(desc.enumerable, false);
assertEq(desc.configurable, true);
assertEq(desc.writable, true);



delete this.Reflect;
assertEq("Reflect" in this, false);

reportCompare(0, 0, 'ok');
