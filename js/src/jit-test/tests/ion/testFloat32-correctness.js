setJitCompilerOption("ion.usecount.trigger", 50);

var f32 = new Float32Array(10);

function test(setup, f) {
    if (f === undefined) {
        f = setup;
        setup = function(){};
    }
    setup();
    for(var n = 200; n; --n) {
        f();
    }
}


function setupBasicArith() {
    f32[0] = -Infinity;
    f32[1] = -1;
    f32[2] = -0;
    f32[3] = 0;
    f32[4] = 1.337;
    f32[5] = 42;
    f32[6] = Infinity;
    f32[7] = NaN;
}
function basicArith() {
    for (var i = 0; i < 7; ++i) {
        var opf = Math.fround(f32[i] + f32[i+1]);
        var opd = (1 / (1 / f32[i])) + f32[i+1];
        assertFloat32(opf, true);
        assertFloat32(opd, false);
        assertEq(opf, Math.fround(opd));

        opf = Math.fround(f32[i] - f32[i+1]);
        opd = (1 / (1 / f32[i])) - f32[i+1];
        assertFloat32(opf, true);
        assertFloat32(opd, false);
        assertEq(opf, Math.fround(opd));

        opf = Math.fround(f32[i] * f32[i+1]);
        opd = (1 / (1 / f32[i])) * f32[i+1];
        assertFloat32(opf, true);
        assertFloat32(opd, false);
        assertEq(opf, Math.fround(opd));

        opf = Math.fround(f32[i] / f32[i+1]);
        opd = (1 / (1 / f32[i])) / f32[i+1];
        assertFloat32(opf, true);
        assertFloat32(opd, false);
        assertEq(opf, Math.fround(opd));
    }
}
test(setupBasicArith, basicArith);


function setupAbs() {
    f32[0] = -0;
    f32[1] = 0;
    f32[2] = -3.14159;
    f32[3] = 3.14159;
    f32[4] = -Infinity;
    f32[5] = Infinity;
    f32[6] = NaN;
}
function abs() {
    for(var i = 0; i < 7; ++i) {
        assertEq( Math.fround(Math.abs(f32[i])), Math.abs(f32[i]) );
    }
}
test(setupAbs, abs);


function setupSqrt() {
    f32[0] = 0;
    f32[1] = 1;
    f32[2] = 4;
    f32[3] = -1;
    f32[4] = Infinity;
    f32[5] = NaN;
    f32[6] = 13.37;
}
function sqrt() {
    for(var i = 0; i < 7; ++i) {
        var sqrtf = Math.fround(Math.sqrt(f32[i]));
        var sqrtd = 1 + Math.sqrt(f32[i]) - 1; 
        assertEq( sqrtf, Math.fround(sqrtd) );
    }
}
test(setupSqrt, sqrt);



function setupTruncateToInt32() {
    f32[0] = -1;
    f32[1] = 4;
    f32[2] = 5.13;
}
function truncateToInt32() {
    assertEq( Math.imul(f32[0], f32[1]), Math.imul(-1, 4) );
    assertEq( Math.imul(f32[1], f32[2]), Math.imul(4, 5) );
}
test(setupTruncateToInt32, truncateToInt32);


function comp() {
    for(var i = 0; i < 9; ++i) {
        assertEq( f32[i] < f32[i+1], true );
    }
}
function setupComp() {
    f32[0] = -Infinity;
    f32[1] = -1;
    f32[2] = -0.01;
    f32[3] = 0;
    f32[4] = 0.01;
    f32[5] = 1;
    f32[6] = 10;
    f32[7] = 13.37;
    f32[8] = 42;
    f32[9] = Infinity;
}
test(setupComp, comp);


function setupNot() {
    f32[0] = -0;
    f32[1] = 0;
    f32[2] = 1;
    f32[3] = NaN;
    f32[4] = Infinity;
    f32[5] = 42;
    f32[5] = -23;
}
function not() {
    assertEq( !f32[0], true );
    assertEq( !f32[1], true );
    assertEq( !f32[2], false );
    assertEq( !f32[3], true );
    assertEq( !f32[4], false );
    assertEq( !f32[5], false );
    assertEq( !f32[6], false );
}
test(setupNot, not);


var str = "can haz cheezburger? okthxbye;";
function setupToInt32() {
    f32[0] = 0;
    f32[1] = 1;
    f32[2] = 2;
    f32[3] = 4;
    f32[4] = 5;
}
function testToInt32() {
    assertEq(str[f32[0]], 'c');
    assertEq(str[f32[1]], 'a');
    assertEq(str[f32[2]], 'n');
    assertEq(str[f32[3]], 'h');
    assertEq(str[f32[4]], 'a');
}
test(setupToInt32, testToInt32);

function setupBailoutToInt32() {
    f32[0] = .5;
}
function testBailoutToInt32() {
    assertEq(typeof str[f32[0]], 'undefined');
}
test(setupBailoutToInt32, testBailoutToInt32);

