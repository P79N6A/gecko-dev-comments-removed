var x = new ArrayBuffer();

x.__proto__ = null;
assertEq(x.__proto__, undefined);

x.__proto__ = null;
assertEq(x.__proto__, null);

x.__proto__ = {a:2};
assertEq(x.__proto__.a, 2);
assertEq(x.a, undefined);

var ab = new ArrayBuffer();

ab.__proto__ = Object.create(null);
ab.__proto__ = {a:2};

assertEq(ab.a, undefined);
reportCompare(true, true);
