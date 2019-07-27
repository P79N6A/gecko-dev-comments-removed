

load(libdir + "asserts.js");
load(libdir + "iteration.js");

var non_iterable1 = {};
non_iterable1[std_iterator] = {};
assertThrowsInstanceOf(() => new WeakMap(non_iterable1), TypeError);

var non_iterable2 = {};
non_iterable2[std_iterator] = function() {
};
assertThrowsInstanceOf(() => new WeakMap(non_iterable2), TypeError);
