

load(libdir + "asserts.js");
var it = [1, 2].iterator();
var v = Object.create(it);
assertThrowsInstanceOf(function () { Iterator.prototype.next.call(v); }, TypeError);
