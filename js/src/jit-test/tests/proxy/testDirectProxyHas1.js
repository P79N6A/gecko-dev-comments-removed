
var proxy = new Proxy(Object.create(Object.create(null, {
    'foo': {
        configurable: true
    }
}), {
    'bar': {
        configurable: true
    }
}), {});
assertEq('foo' in proxy, true);
assertEq('bar' in proxy, true);
assertEq('baz' in proxy, false);
