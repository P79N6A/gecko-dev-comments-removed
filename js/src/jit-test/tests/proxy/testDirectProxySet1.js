
var target = {
    foo: 'bar'
};
new Proxy(target, {})['foo'] = 'baz';
assertEq(target.foo, 'baz');
