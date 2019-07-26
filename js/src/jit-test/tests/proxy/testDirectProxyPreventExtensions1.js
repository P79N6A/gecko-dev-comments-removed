
target = {};
var proxy = new Proxy(target, {});
Object.preventExtensions(proxy);
assertEq(Object.isExtensible(target), false);
assertEq(Object.isExtensible(proxy), false);
