
var proxy = Proxy(Object.create(Object.create(null, {
    'foo': {
        configurable: true
    }
}), {
    'bar': {
        configurable: true
    }
}), {});
assertEq(({}).hasOwnProperty.call(proxy, 'foo'), false);
assertEq(({}).hasOwnProperty.call(proxy, 'bar'), true);



var called = false;
var handler = { getOwnPropertyDescriptor: function () { called = true; },
                has: function () { assertEq(false, true, "has trap must not be called"); }
              }
proxy = new Proxy({}, handler);
assertEq(({}).hasOwnProperty.call(proxy, 'foo'), false);
assertEq(called, true);
