load(libdir + "asserts.js");


assertThrowsInstanceOf(function () { Proxy(); }, TypeError);
assertThrowsInstanceOf(function () { new Proxy(); }, TypeError);
assertThrowsInstanceOf(function () { Proxy({}); }, TypeError);
assertThrowsInstanceOf(function () { new Proxy({}); }, TypeError);


assertThrowsInstanceOf(function () { Proxy(0, {}); }, TypeError);
assertThrowsInstanceOf(function () { new Proxy(0, {}); }, TypeError);
assertThrowsInstanceOf(function () { Proxy(null, {}); }, TypeError);
assertThrowsInstanceOf(function () { new Proxy(null, {}); }, TypeError);


assertThrowsInstanceOf(function () { Proxy({}, 0); }, TypeError);
assertThrowsInstanceOf(function () { new Proxy({}, 0); }, TypeError);
assertThrowsInstanceOf(function () { Proxy({}, null); }, TypeError);
assertThrowsInstanceOf(function () { new Proxy({}, null); }, TypeError);


assertEq(typeof Proxy({}, {}), 'object');
assertEq(typeof new Proxy({}, {}), 'object');
