






var BUGNUMBER = 1023145;
var summary = "Use the original getPrototypeOf in self-hosted code";

delete Object.prototype.__proto__;
var int32x4 = SIMD.int32x4;
var ar = int32x4.array(1);
var array = new ar([int32x4(1, 2, 3, 4)]);

if (typeof reportCompare === "function")
    reportCompare(true, true);
