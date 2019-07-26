

load(libdir + "asserts.js");
load(libdir + "iteration.js");

var it = [1, 2][std_iterator]();
var v = Object.create(it);
assertThrowsInstanceOf(function () { Iterator.prototype.next.call(v); }, TypeError);
