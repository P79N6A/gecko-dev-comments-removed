

load(libdir + "asserts.js");

delete this.arguments;  
var iter = (arguments for (x of [1]));
assertThrowsInstanceOf(() => iter.next(), ReferenceError);
