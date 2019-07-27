










const POISON = 42;

function reset(ta) {
    for (var i = 0; i < ta.length; i++)
        ta[i] = POISON + i;
}

function assertChanged(ta, from, expected) {
    var i = 0;
    for (; i < from; i++)
        assertEq(ta[i], POISON + i);
    for (; i < from + expected.length; i++)
        assertEq(ta[i], expected[i - from]);
    for (; i < ta.length; i++)
        assertEq(ta[i], POISON + i);
}

function testStore(ta, kind, i, v) {
    reset(ta);
    SIMD[kind].storeX(ta, i, v);
    assertChanged(ta, i, [v.x]);

    reset(ta);
    SIMD[kind].storeXY(ta, i, v);
    assertChanged(ta, i, [v.x, v.y]);

    reset(ta);
    SIMD[kind].storeXYZ(ta, i, v);
    assertChanged(ta, i, [v.x, v.y, v.z]);

    reset(ta);
    SIMD[kind].store(ta, i, v);
    assertChanged(ta, i, [v.x, v.y, v.z, v.w]);
}

function testStoreInt32x4() {
    var I32 = new Int32Array(16);

    var v = SIMD.int32x4(0, 1, Math.pow(2,31) - 1, -Math.pow(2, 31));
    testStore(I32, 'int32x4', 0, v);
    testStore(I32, 'int32x4', 1, v);
    testStore(I32, 'int32x4', 2, v);
    testStore(I32, 'int32x4', 12, v);

    assertThrowsInstanceOf(() => SIMD.int32x4.store(I32), TypeError);
    assertThrowsInstanceOf(() => SIMD.int32x4.store(I32, 0), TypeError);
    assertThrowsInstanceOf(() => SIMD.float32x4.store(I32, 0, v), TypeError);
}

function testStoreFloat32x4() {
    var F32 = new Float32Array(16);

    var v = SIMD.float32x4(1,2,3,4);
    testStore(F32, 'float32x4', 0, v);
    testStore(F32, 'float32x4', 1, v);
    testStore(F32, 'float32x4', 2, v);
    testStore(F32, 'float32x4', 12, v);

    var v = SIMD.float32x4(NaN, -0, -Infinity, 5e-324);
    testStore(F32, 'float32x4', 0, v);
    testStore(F32, 'float32x4', 1, v);
    testStore(F32, 'float32x4', 2, v);
    testStore(F32, 'float32x4', 12, v);

    assertThrowsInstanceOf(() => SIMD.float32x4.store(F32), TypeError);
    assertThrowsInstanceOf(() => SIMD.float32x4.store(F32, 0), TypeError);
    assertThrowsInstanceOf(() => SIMD.int32x4.store(F32, 0, v), TypeError);
}

testStoreInt32x4();
testStoreFloat32x4();

if (typeof reportCompare === "function")
    reportCompare(true, true);
