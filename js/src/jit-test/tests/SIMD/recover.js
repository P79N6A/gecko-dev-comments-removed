load(libdir + 'simd.js');

if (!this.hasOwnProperty("SIMD"))
  quit();

setJitCompilerOption("baseline.warmup.trigger", 10);
setJitCompilerOption("ion.warmup.trigger", 20);




var uceFault = function (i) {
    if (i > 98)
        uceFault = function (i) { return true; };
    return false;
};


var uceFault_simdBox_i4 = eval(uneval(uceFault).replace('uceFault', 'uceFault_simdBox_i4'));
function simdBox_i4(i) {
    var a = SIMD.int32x4(i, i, i, i);
    if (uceFault_simdBox_i4(i) || uceFault_simdBox_i4(i)) {
        assertEqX4(a, [i, i, i, i]);
    }
    return 0;
}

var uceFault_simdBox_f4 = eval(uneval(uceFault).replace('uceFault', 'uceFault_simdBox_f4'));
function simdBox_f4(i) {
    var a = SIMD.float32x4(i, i + 0.1, i + 0.2, i + 0.3);
    if (uceFault_simdBox_f4(i) || uceFault_simdBox_f4(i)) {
        assertEqX4(a, [i, i + 0.1, i + 0.2, i + 0.3].map(Math.fround));
    }
    return 0;
}

for (var i = 0; i < 100; i++) {
    simdBox_i4(i);
    simdBox_f4(i);
}
