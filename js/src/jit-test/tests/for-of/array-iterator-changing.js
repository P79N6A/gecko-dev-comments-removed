

load(libdir + "asserts.js");
load(libdir + "iteration.js");

var arr = [0, 1, 2];
var it = arr[std_iterator]();
arr[0] = 1000;
arr[2] = 2000;
assertIteratorNext(it, 1000);
assertIteratorNext(it, 1);
assertIteratorNext(it, 2000);
assertIteratorDone(it, undefined);
