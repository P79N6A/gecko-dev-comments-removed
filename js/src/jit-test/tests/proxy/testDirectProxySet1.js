
var target = {
    foo: 'bar'
};
Proxy(target, {})['foo'] = 'baz';
assertEq(target.foo, 'baz');
