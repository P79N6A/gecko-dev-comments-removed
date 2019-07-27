









var Int32x4 = SIMD.Int32x4;
var a = Int32x4((4294967295), 200, 300, 400);
var c = SIMD.Float32x4.fromInt32x4Bits(a);


assertEq(c.x, NaN);



var d = SIMD.Int32x4.fromFloat32x4Bits(c);
assertEq(d.x, -1);

if (typeof reportCompare === "function")
    reportCompare(true, true);
