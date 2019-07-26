







load(libdir + "asserts.js");
load(libdir + "iteration.js");

delete Array.prototype[std_iterator];
assertThrowsInstanceOf(function () { for (var x of []) ; }, TypeError);
