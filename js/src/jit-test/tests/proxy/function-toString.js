load(libdir + 'asserts.js');



var proxy = new Proxy(function() {}, {});
assertThrowsInstanceOf(() => Function.prototype.toString.call(proxy), TypeError);
var o = Proxy.revocable(function() {}, {});
assertThrowsInstanceOf(() => Function.prototype.toString.call(o.proxy), TypeError);
o.revoke();
assertThrowsInstanceOf(() => Function.prototype.toString.call(o.proxy), TypeError);
