

load(libdir + "asserts.js");
delete String.prototype.iterator;
assertThrowsInstanceOf(function () { for (var v of "abc") ; }, TypeError);
assertThrowsInstanceOf(function () { for (var v of new String("abc")) ; }, TypeError);
