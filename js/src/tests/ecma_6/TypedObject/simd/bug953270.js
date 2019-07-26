






var BUGNUMBER = 953270;
var summary = 'Handles';




var int32x4 = SIMD.int32x4;
var a = int32x4((4294967295), 200, 300, 400);
var c = SIMD.int32x4.bitsToFloat32x4(a);


assertEq(c.x, NaN);



var d = SIMD.float32x4.bitsToInt32x4(c);
assertEq(d.x, -1);

reportCompare(true, true);
print("Tests complete");
