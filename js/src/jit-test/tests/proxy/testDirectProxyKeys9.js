
var names = Object.keys(new Proxy(Object.create(Object.create(null, {
    a: {
        enumerable: true,
        configurable: true
    },
    b: {
        enumerable: false,
        configurable: true
    }
}), {
    c: {
        enumerable: true,
        configurable: true
    },
    d: {
        enumerable: false,
        configurable: true
    }
}), {
    ownKeys: function (target) {
        return [ 'e' ];
    }
}));
assertEq(names.length, 0);
