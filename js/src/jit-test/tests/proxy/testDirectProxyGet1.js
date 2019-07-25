
assertEq(new Proxy({
    foo: 'bar'
}, {}).foo, 'bar');

assertEq(new Proxy({
    foo: 'bar'
}, {})['foo'], 'bar');
