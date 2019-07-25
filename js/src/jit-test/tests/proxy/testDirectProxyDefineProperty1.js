
var target = {};
Object.defineProperty(new Proxy(target, {}), 'foo', {
    value: 'bar',
    writable: true,
    enumerable: false,
    configurable: true
});
var desc = Object.getOwnPropertyDescriptor(target, 'foo');
assertEq(desc.value, 'bar');
assertEq(desc.writable, true);
assertEq(desc.enumerable, false);
assertEq(desc.configurable, true);
