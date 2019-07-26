







load(libdir + "asserts.js");
delete Array.prototype.iterator;
assertThrowsInstanceOf(function () { for (var x of []) ; }, TypeError);
