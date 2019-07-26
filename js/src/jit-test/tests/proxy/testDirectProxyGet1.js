
assertEq(Proxy({
    foo: 'bar'
}, {}).foo, 'bar');

assertEq(Proxy({
    foo: 'bar'
}, {})['foo'], 'bar');
