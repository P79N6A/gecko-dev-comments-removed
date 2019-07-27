






var BUGNUMBER = 953270;
var summary = 'Handles';




var int32x4 = SIMD.int32x4;
var a = int32x4((4294967295), 200, 300, 400);
var c = SIMD.float32x4.fromInt32x4Bits(a);


assertEq(c.x, NaN);



var d = SIMD.int32x4.fromFloat32x4Bits(c);
assertEq(d.x, -1);

reportCompare(true, true);
print("Tests complete");
