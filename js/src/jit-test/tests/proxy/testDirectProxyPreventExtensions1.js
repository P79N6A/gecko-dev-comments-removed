
var target = {};
Object.preventExtensions(new Proxy(target, {}));
assertEq(Object.isExtensible(target), false);
