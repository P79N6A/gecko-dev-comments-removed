



var m = new WeakMap;
var sym = Symbol();
assertThrowsInstanceOf(() => m.set(sym, 0), TypeError);

if (typeof reportCompare === "function")
  reportCompare(0, 0);
