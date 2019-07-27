load(libdir + "asm.js");
var heap = new ArrayBuffer(4096);


const DEBUG = false;

if (!isSimdAvailable() || typeof SIMD === 'undefined') {
    DEBUG && print("won't run tests as simd extensions aren't activated yet");
    quit(0);
}

const I32 = 'var i4 = glob.SIMD.int32x4;'
const I32A = 'var i4a = i4.add;'
const I32S = 'var i4s = i4.sub;'
const F32 = 'var f4 = glob.SIMD.float32x4;'
const F32A = 'var f4a = f4.add;'
const F32S = 'var f4s = f4.sub;'
const F32M = 'var f4m = f4.mul;'
const F32D = 'var f4d = f4.div;'
const FROUND = 'var f32=glob.Math.fround;'

const INT32_MAX = Math.pow(2, 31) - 1;
const INT32_MIN = INT32_MAX + 1 | 0;

const assertEqFFI = {assertEq:assertEq};

function assertEqX4(real, expected) {
    assertEq(real.x, expected[0]);
    assertEq(real.y, expected[1]);
    assertEq(real.z, expected[2]);
    assertEq(real.w, expected[3]);
}

function CheckI4(header, code, expected) {
    
    header = USE_ASM + I32 + header;
    var lanes = ['x', 'y', 'z', 'w'];
    for (var i = 0; i < 4; ++i) {
        var lane = lanes[i];
        assertEq(asmLink(asmCompile('glob', header + ';function f() {' + code + ';return x.' + lane + '|0} return f'), this)(), expected[i]);
    }
}

function CheckF4(header, code, expected) {
    
    var lanes = ['x', 'y', 'z', 'w'];
    header = USE_ASM + F32 + header;
    for (var i = 0; i < 4; ++i) {
        var lane = lanes[i];
        assertEq(asmLink(asmCompile('glob', header + ';function f() {' + code + ';return +x.' + lane + '} return f'), this)(), Math.fround(expected[i]));
    }
}

try {




assertAsmTypeFail('glob', USE_ASM + "var i4 = int32x4               ; return {}") ;
assertAsmTypeFail('glob', USE_ASM + "var i4 = glob.int32x4          ; return {}") ;
assertAsmTypeFail('glob', USE_ASM + "var i4 = glob.globglob.int32x4 ; return {}") ;
assertAsmTypeFail('glob', USE_ASM + "var i4 = glob.Math.int32x4     ; return {}") ;
assertAsmTypeFail('glob', USE_ASM + "var herd = glob.SIMD.ponyX4    ; return {}") ;


assertAsmLinkAlwaysFail(asmCompile('glob', USE_ASM + I32 + "return {}"), {});
assertAsmLinkFail(asmCompile('glob', USE_ASM + I32 + "return {}"), {SIMD: 42});
assertAsmLinkFail(asmCompile('glob', USE_ASM + I32 + "return {}"), {SIMD: Math.fround});
assertAsmLinkFail(asmCompile('glob', USE_ASM + I32 + "return {}"), {SIMD: {}});
assertAsmLinkFail(asmCompile('glob', USE_ASM + I32 + "return {}"), {SIMD: {int32x4: 42}});
assertAsmLinkFail(asmCompile('glob', USE_ASM + I32 + "return {}"), {SIMD: {int32x4: Math.fround}});
assertAsmLinkFail(asmCompile('glob', USE_ASM + I32 + "return {}"), {SIMD: {int32x4: new Array}});
assertAsmLinkFail(asmCompile('glob', USE_ASM + I32 + "return {}"), {SIMD: {int32x4: SIMD.float32x4}});

[Type, int32] = [TypedObject.StructType, TypedObject.int32];
var MyStruct = new Type({'x': int32, 'y': int32, 'z': int32, 'w': int32});
assertAsmLinkFail(asmCompile('glob', USE_ASM + I32 + "return {}"), {SIMD: {int32x4: MyStruct}});
assertAsmLinkFail(asmCompile('glob', USE_ASM + I32 + "return {}"), {SIMD: {int32x4: new MyStruct}});

assertEq(asmLink(asmCompile('glob', USE_ASM + I32 + "function f() {} return f"), {SIMD:{int32x4: SIMD.int32x4}})(), undefined);

assertAsmLinkFail(asmCompile('glob', USE_ASM + F32 + "return {}"), {SIMD: {float32x4: 42}});
assertAsmLinkFail(asmCompile('glob', USE_ASM + F32 + "return {}"), {SIMD: {float32x4: Math.fround}});
assertAsmLinkFail(asmCompile('glob', USE_ASM + F32 + "return {}"), {SIMD: {float32x4: new Array}});
assertEq(asmLink(asmCompile('glob', USE_ASM + F32 + "function f() {} return f"), {SIMD:{float32x4: SIMD.float32x4}})(), undefined);



assertAsmTypeFail('glob', USE_ASM + "function f() {var x=Int32x4(1,2,3,4);} return f");
assertAsmTypeFail('glob', USE_ASM + I32 + "function f() {var x=i4;} return f");
assertAsmTypeFail('glob', USE_ASM + I32 + "function f() {var x=i4();} return f");
assertAsmTypeFail('glob', USE_ASM + I32 + "function f() {var x=i4(1);} return f");
assertAsmTypeFail('glob', USE_ASM + I32 + "function f() {var x=i4(1, 2);} return f");
assertAsmTypeFail('glob', USE_ASM + I32 + "function f() {var x=i4(1, 2, 3);} return f");
assertAsmTypeFail('glob', USE_ASM + I32 + "function f() {var x=i4(1, 2, 3, 4.0);} return f");
assertAsmTypeFail('glob', USE_ASM + I32 + "function f() {var x=i4(1, 2.0, 3, 4);} return f");
assertAsmTypeFail('glob', USE_ASM + I32 + I32A + "function f() {var x=i4a(1,2,3,4);} return f");
assertAsmTypeFail('glob', USE_ASM + I32 + "function f() {var x=i4(1,2,3,2+2|0);} return f");
assertAsmTypeFail('glob', USE_ASM + I32 + "function f() {var x=i4(1,2,3," + (INT32_MIN - 1) + ");} return f");
assertAsmTypeFail('glob', USE_ASM + I32 + "function f() {var x=i4(i4(1,2,3,4));} return f");
assertEq(asmLink(asmCompile('glob', USE_ASM + I32 + "function f() {var x=i4(1,2,3,4);} return f"), this)(), undefined);
assertEq(asmLink(asmCompile('glob', USE_ASM + I32 + "function f() {var x=i4(1,2,3," + (INT32_MAX + 1) + ");} return f"), this)(), undefined);

assertAsmTypeFail('glob', USE_ASM + F32 + "function f() {var x=f4;} return f");
assertAsmTypeFail('glob', USE_ASM + F32 + "function f() {var x=f4();} return f");
assertAsmTypeFail('glob', USE_ASM + F32 + "function f() {var x=f4(1,2,3);} return f");
assertAsmTypeFail('glob', USE_ASM + F32 + "function f() {var x=f4(1.,2.,3.);} return f");
assertAsmTypeFail('glob', USE_ASM + F32 + "function f() {var x=f4(1.,2.,f32(3.),4.);} return f");
assertEq(asmLink(asmCompile('glob', USE_ASM + F32 + "function f() {var x=f4(1.,2.,3.,4.);} return f"), this)(), undefined);
assertEq(asmLink(asmCompile('glob', USE_ASM + F32 + "function f() {var x=f4(1,2,3,4);} return f"), this)(), undefined);
assertEq(asmLink(asmCompile('glob', USE_ASM + F32 + "function f() {var x=f4(1,2,3," + (INT32_MIN - 1) + ");} return f"), this)(), undefined);
assertEq(asmLink(asmCompile('glob', USE_ASM + F32 + "function f() {var x=f4(1,2,3," + (INT32_MAX + 1) + ");} return f"), this)(), undefined);


assertAsmTypeFail('glob', USE_ASM + I32 + "function f(i) {i=i|0; var z=0; switch(i|0) {case i4(1,2,3,4): z=1; break; default: z=2; break;}} return f");
assertAsmTypeFail('glob', USE_ASM + I32 + "function f(i) {i=i|0; var z=0; return i * i4(1,2,3,4) | 0;} return f");
assertAsmTypeFail('glob', USE_ASM + I32 + "function f(i) {var x=i4(1,2,3,i4(4,5,6,7))} return f");
assertAsmTypeFail('glob', USE_ASM + I32 + F32 + "function f(i) {var x=i4(1,2,3,f4(4,5,6,7))} return f");
assertAsmTypeFail('glob', USE_ASM + I32 + F32 + "function f(i) {var x=f4(1,2,3,i4(4,5,6,7))} return f");

assertEqX4(asmLink(asmCompile('glob', USE_ASM + I32 + "function f() {return i4(1,2,3,4);} return f"), this)(), [1, 2, 3, 4]);
assertEqX4(asmLink(asmCompile('glob', USE_ASM + I32 + "function f() {return i4(i4(1,2,3,4));} return f"), this)(), [1, 2, 3, 4]);
assertEqX4(asmLink(asmCompile('glob', USE_ASM + F32 + "function f() {return f4(1,2,3,4);} return f"), this)(), [1, 2, 3, 4]);
assertEqX4(asmLink(asmCompile('glob', USE_ASM + F32 + "function f() {return f4(f4(1,2,3,4));} return f"), this)(), [1, 2, 3, 4]);


assertEqX4(asmLink(asmCompile('glob', 'ffi', 'heap', USE_ASM + I32 + "var i32=new glob.Int32Array(heap); function f(i) {i=i|0; return i4(i4(i32[i>>2], 2, 3, 4))} return f"), this, {}, new ArrayBuffer(0x10000))(0x20000), [0, 2, 3, 4]);

assertEqX4(asmLink(asmCompile('glob', 'ffi', 'heap', USE_ASM + F32 + "var f32=new glob.Float32Array(heap); function f(i) {i=i|0; return f4(f4(f32[i>>2], 2, 3, 4))} return f"), this, {}, new ArrayBuffer(0x10000))(0x20000), [NaN, 2, 3, 4]);
assertEqX4(asmLink(asmCompile('glob', USE_ASM + F32 + "var f32=glob.Math.fround; function f(i) {i=i|0; return f4(f4(f32(1) + f32(2), 2, 3, 4))} return f"), this, {}, new ArrayBuffer(0x10000))(0x20000), [3, 2, 3, 4]);


assertAsmTypeFail('glob', USE_ASM + "function f() {var x=1; return x.y | 0;} return f");
assertAsmTypeFail('glob', USE_ASM + "function f() {var x=1; return (x + x).y | 0;} return f");
assertAsmTypeFail('glob', USE_ASM + "function f() {var x=1.; return x.y | 0;} return f");
assertAsmTypeFail('glob', USE_ASM + "var f32=glob.Math.fround;" + I32 + "function f() {var x=f32(1); return x.y | 0;} return f");

assertAsmTypeFail('glob', USE_ASM + I32 + "function f() {var x=i4(1,2,3,4); return x.length|0;} return f");
assertAsmTypeFail('glob', USE_ASM + I32 + "function f() {var x=i4(1,2,3,4).y; return x|0;} return f");

CheckI4('', 'var x=i4(0,0,0,0)', [0,0,0,0]);
CheckI4('', 'var x=i4(1,2,3,4)', [1,2,3,4]);
CheckI4('', 'var x=i4(' + INT32_MIN + ',2,3,' + INT32_MAX + ')', [INT32_MIN,2,3,INT32_MAX]);
CheckI4('', 'var x=i4(1,2,3,4); var y=i4(5,6,7,8)', [1,2,3,4]);
CheckI4('', 'var a=1; var b=i4(9,8,7,6); var c=13.37; var x=i4(1,2,3,4); var y=i4(5,6,7,8)', [1,2,3,4]);
CheckI4('', 'var y=i4(5,6,7,8); var x=i4(1,2,3,4)', [1,2,3,4]);

CheckF4('', 'var x=f4(' + INT32_MAX + ', 2, 3, ' + INT32_MIN + ')', [INT32_MAX, 2, 3, INT32_MIN]);
CheckF4('', 'var x=f4(' + (INT32_MAX + 1) + ', 2, 3, 4)', [INT32_MAX + 1, 2, 3, 4]);
CheckF4('', 'var x=f4(1.3, 2.4, 3.5, 98.76)', [1.3, 2.4, 3.5, 98.76]);
CheckF4('', 'var x=f4(13.37, 2., 3., -0)', [13.37, 2, 3, -0]);


assertAsmTypeFail('glob', USE_ASM + I32 + I32A + "function f() {var x=i4(1,2,3,4); x=i4();} return f");
assertAsmTypeFail('glob', USE_ASM + I32 + I32A + "function f() {var x=i4(1,2,3,4); x=i4(1);} return f");
assertAsmTypeFail('glob', USE_ASM + I32 + I32A + "function f() {var x=i4(1,2,3,4); x=i4(1, 2);} return f");
assertAsmTypeFail('glob', USE_ASM + I32 + I32A + "function f() {var x=i4(1,2,3,4); x=i4(1, 2, 3);} return f");
assertAsmTypeFail('glob', USE_ASM + I32 + I32A + "function f() {var x=i4(1,2,3,4); x=i4(1.0, 2, 3, 4);} return f");
assertAsmTypeFail('glob', USE_ASM + I32 + I32A + "function f() {var x=i4(1,2,3,4); x=i4(1, 2.0, 3, 4);} return f");
assertAsmTypeFail('glob', USE_ASM + I32 + I32A + "function f() {var x=i4(1,2,3,4); x=i4(1, 2, 3.0, 4);} return f");
assertAsmTypeFail('glob', USE_ASM + I32 + I32A + "function f() {var x=i4(1,2,3,4); x=i4(1, 2, 3, 4.0);} return f");
assertAsmTypeFail('glob', USE_ASM + I32 + I32A + "function f() {var x=i4(1,2,3,4); x=i4(1, 2, 3, x);} return f");
assertAsmTypeFail('glob', USE_ASM + I32 + I32A + "function f() {var x=i4(1,2,3,4); var c=4.0; x=i4(1, 2, 3, +c);} return f");

assertAsmTypeFail('glob', 'ffi', 'heap', USE_ASM + I32 + "var i32=new glob.Int32Array(heap); function f() {var x=i4(1,2,3,4); i32[0] = x;} return f");
assertAsmTypeFail('glob', 'ffi', 'heap', USE_ASM + I32 + "var i32=new glob.Int32Array(heap); function f() {var x=i4(1,2,3,4); x = i32[0];} return f");
assertAsmTypeFail('glob', 'ffi', 'heap', USE_ASM + F32 + "var f32=new glob.Float32Array(heap); function f() {var x=f4(1,2,3,4); f32[0] = x;} return f");
assertAsmTypeFail('glob', 'ffi', 'heap', USE_ASM + F32 + "var f32=new glob.Int32Array(heap); function f() {var x=f4(1,2,3,4); x = f32[0];} return f");

CheckI4('', 'var x=i4(1,2,3,4); x=i4(5,6,7,8)', [5, 6, 7, 8]);
CheckI4('', 'var x=i4(1,2,3,4); var c=6; x=i4(5,c|0,7,8)', [5, 6, 7, 8]);
CheckI4('', 'var x=i4(8,7,6,5); x=i4(x.w|0,x.z|0,x.y|0,x.x|0)', [5, 6, 7, 8]);

CheckF4(FROUND, 'var x=f4(1,2,3,4); var y=f32(7.); x=f4(5,6,y,8)', [5, 6, 7, 8]);
CheckF4(FROUND, 'var x=f4(1,2,3,4); x=f4(f32(5.),6.,7.,8.)', [5, 6, 7, 8]);
CheckF4(FROUND, 'var x=f4(1,2,3,4); x=f4(f32(5),6,7,8)', [5, 6, 7, 8]);
CheckF4(FROUND, 'var x=f4(1,2,3,4); x=f4(f32(5.),f32(6.),f32(7.),f32(8.))', [5, 6, 7, 8]);
CheckF4('', 'var x=f4(1.,2.,3.,4.); x=f4(5.,6.,7.,8.)', [5, 6, 7, 8]);
CheckF4('', 'var x=f4(1.,2.,3.,4.); x=f4(1,2,3,4)', [1, 2, 3, 4]);
CheckF4(FROUND, 'var x=f4(1.,2.,3.,4.); var y=f32(7.); x=f4(9, 4, 2, 1)', [9, 4, 2, 1]);
CheckF4('', 'var x=f4(8.,7.,6.,5.); x=f4(x.w, x.z, x.y, x.x)', [5, 6, 7, 8]);


CheckI4('', 'var x=i4(1,2,3,4); var c=6; x=i4(c|0,c|0,c|0,c|0)', [6, 6, 6, 6]);
CheckF4(FROUND, 'var x=f4(1,2,3,4); var y=f32(7.); x=f4(y,y,y,y)', [7, 7, 7, 7]);


assertAsmTypeFail('glob', USE_ASM + I32 + "function f() {var x=1; return i4(x)} return f");
assertAsmTypeFail('glob', USE_ASM + I32 + "function f() {var x=1; return i4(x + x)} return f");
assertAsmTypeFail('glob', USE_ASM + I32 + "function f() {var x=1.; return i4(x)} return f");
assertAsmTypeFail('glob', USE_ASM + I32 + FROUND + "function f() {var x=f32(1.); return i4(x)} return f");

assertEqX4(asmLink(asmCompile('glob', USE_ASM + I32 + "function f() {var x=i4(1,2,3,4); return i4(x)} return f"), this)(), [1,2,3,4]);
assertEqX4(asmLink(asmCompile('glob', USE_ASM + F32 + "function f() {var x=f4(1,2,3,4); return f4(x)} return f"), this)(), [1,2,3,4]);


assertAsmTypeFail('glob', USE_ASM + I32 + "function f(x) {x=i4();} return f");
assertAsmTypeFail('glob', USE_ASM + I32 + "function f(x) {x=i4(1,2,3,4);} return f");
assertAsmTypeFail('glob', USE_ASM + I32 + "function f(x,y) {x=i4(y);y=+y} return f");

var i32x4 = SIMD.int32x4(1, 3, 3, 7);
assertEq(asmLink(asmCompile('glob', USE_ASM + I32 + "function f(x) {x=i4(x)} return f"), this)(i32x4), undefined);
assertEqX4(asmLink(asmCompile('glob', USE_ASM + I32 + "function f(x) {x=i4(x); return i4(x);} return f"), this)(i32x4), [1,3,3,7]);

var f32x4 = SIMD.float32x4(13.37, 42.42, -0, NaN);
assertEq(asmLink(asmCompile('glob', USE_ASM + F32 + "function f(x) {x=f4(x)} return f"), this)(f32x4), undefined);
assertEqX4(asmLink(asmCompile('glob', USE_ASM + F32 + "function f(x) {x=f4(x); return f4(x);} return f"), this)(f32x4),
           [Math.fround(13.37), Math.fround(42.42), -0, NaN]);

function assertCaught(f) {
    var caught = false;
    try {
        f.apply(null, Array.prototype.slice.call(arguments, 1));
    } catch (e) {
        DEBUG && print('Assert caught: ', e, '\n', e.stack);
        assertEq(e instanceof TypeError, true);
        caught = true;
    }
    assertEq(caught, true);
}

var f = asmLink(asmCompile('glob', USE_ASM + F32 + "function f(x) {x=f4(x); return f4(x);} return f"), this);
assertCaught(f);
assertCaught(f, 1);
assertCaught(f, {});
assertCaught(f, "I sincerely am a SIMD typed object.");
assertCaught(f, SIMD.int32x4(1,2,3,4));

var f = asmLink(asmCompile('glob', USE_ASM + I32 + "function f(x) {x=i4(x); return i4(x);} return f"), this);
assertCaught(f);
assertCaught(f, 1);
assertCaught(f, {});
assertCaught(f, "I sincerely am a SIMD typed object.");
assertCaught(f, SIMD.float32x4(4,3,2,1));




assertAsmTypeFail('glob', USE_ASM + I32 + "var g=i4(1,2,3,4); function f() {var x=4; x=g|0;} return f");
assertAsmTypeFail('glob', USE_ASM + I32 + "var g=i4(1,2,3,4); function f() {var x=4.; x=+g;} return f");
assertAsmTypeFail('glob', USE_ASM + I32 + "var g=i4(1,2,3,4); var f32=glob.Math.fround; function f() {var x=f32(4.); x=f32(g);} return f");

assertAsmTypeFail('glob', USE_ASM + F32 + "var g=f4(1., 2., 3., 4.); function f() {var x=4; x=g|0;} return f");
assertAsmTypeFail('glob', USE_ASM + F32 + "var g=f4(1., 2., 3., 4.); function f() {var x=4.; x=+g;} return f");
assertAsmTypeFail('glob', USE_ASM + F32 + "var g=f4(1., 2., 3., 4.); var f32=glob.Math.fround; function f() {var x=f32(4.); x=f32(g);} return f");

assertAsmTypeFail('glob', USE_ASM + F32 + I32 + "var g=f4(1., 2., 3., 4.); function f() {var x=i4(1,2,3,4); x=i4(g);} return f");
assertAsmTypeFail('glob', USE_ASM + I32 + F32 + "var g=i4(1,2,3,4); function f() {var x=f4(1.,2.,3.,4.); x=f4(g);} return f");

assertAsmTypeFail('glob', USE_ASM + I32 + "var g=0; function f() {var x=i4(1,2,3,4); x=g|0;} return f");
assertAsmTypeFail('glob', USE_ASM + I32 + "var g=0.; function f() {var x=i4(1,2,3,4); x=+g;} return f");
assertAsmTypeFail('glob', USE_ASM + I32 + "var f32=glob.Math.fround; var g=f32(0.); function f() {var x=i4(1,2,3,4); x=f32(g);} return f");

assertAsmTypeFail('glob', USE_ASM + F32 + "var g=0; function f() {var x=f4(0.,0.,0.,0.); x=g|0;} return f");
assertAsmTypeFail('glob', USE_ASM + F32 + "var g=0.; function f() {var x=f4(0.,0.,0.,0.); x=+g;} return f");
assertAsmTypeFail('glob', USE_ASM + F32 + "var f32=glob.Math.fround; var g=f32(0.); function f() {var x=f4(0.,0.,0.,0.); x=f32(g);} return f");

CheckI4('var x=i4(1,2,3,4)', '', [1, 2, 3, 4]);
CheckI4('var _=42; var h=i4(5,5,5,5); var __=13.37; var x=i4(4,7,9,2);', '', [4,7,9,2]);

CheckF4('var x=f4(1.,2.,3.,4.)', '', [1, 2, 3, 4]);
CheckF4('var _=42; var h=f4(5.,5.,5.,5.); var __=13.37; var x=f4(4.,13.37,9.,-0.);', '', [4, 13.37, 9, -0]);
CheckF4('var x=f4(1,2,3,4)', '', [1, 2, 3, 4]);


assertAsmTypeFail('glob', USE_ASM + I32 + "var g=i4(1,2,3,4); function f() {var x=4; g=x|0;} return f");
assertAsmTypeFail('glob', USE_ASM + I32 + "var g=i4(1,2,3,4); function f() {var x=4.; g=+x;} return f");
assertAsmTypeFail('glob', USE_ASM + I32 + "var g=i4(1,2,3,4); var f32=glob.Math.fround; function f() {var x=f32(4.); g=f32(x);} return f");

assertAsmTypeFail('glob', USE_ASM + F32 + "var g=f4(1., 2., 3., 4.); function f() {var x=4; g=x|0;} return f");
assertAsmTypeFail('glob', USE_ASM + F32 + "var g=f4(1., 2., 3., 4.); function f() {var x=4.; g=+x;} return f");
assertAsmTypeFail('glob', USE_ASM + F32 + "var g=f4(1., 2., 3., 4.); var f32=glob.Math.fround; function f() {var x=f32(4.); g=f32(x);} return f");

assertAsmTypeFail('glob', USE_ASM + F32 + I32 + "var g=f4(1., 2., 3., 4.); function f() {var x=i4(1,2,3,4); g=i4(x);} return f");
assertAsmTypeFail('glob', USE_ASM + F32 + I32 + "var g=f4(1., 2., 3., 4.); function f() {var x=i4(1,2,3,4); g=f4(x);} return f");
assertAsmTypeFail('glob', USE_ASM + I32 + F32 + "var g=i4(1,2,3,4); function f() {var x=f4(1.,2.,3.,4.); g=f4(x);} return f");
assertAsmTypeFail('glob', USE_ASM + I32 + F32 + "var g=i4(1,2,3,4); function f() {var x=f4(1.,2.,3.,4.); g=i4(x);} return f");

CheckI4('var x=i4(0,0,0,0);', 'x=i4(1,2,3,4)', [1,2,3,4]);
CheckF4('var x=f4(0.,0.,0.,0.);', 'x=f4(5.,3.,4.,2.)', [5,3,4,2]);

CheckI4('var x=i4(0,0,0,0); var y=42; var z=3.9; var w=13.37', 'x=i4(1,2,3,4); y=24; z=4.9; w=23.10;', [1,2,3,4]);
CheckF4('var x=f4(0,0,0,0); var y=42; var z=3.9; var w=13.37', 'x=f4(1,2,3,4); y=24; z=4.9; w=23.10;', [1,2,3,4]);



var int32x4 = asmLink(asmCompile('glob', 'ffi', USE_ASM + I32 + "var g=i4(ffi.g); function f() {return i4(g)} return f"), this, {g: SIMD.int32x4(1,2,3,4)})();
assertEq(int32x4.x, 1);
assertEq(int32x4.y, 2);
assertEq(int32x4.z, 3);
assertEq(int32x4.w, 4);

for (var v of [1, {}, "totally legit SIMD variable", SIMD.float32x4(1,2,3,4)])
    assertCaught(asmCompile('glob', 'ffi', USE_ASM + I32 + "var g=i4(ffi.g); function f() {return i4(g)} return f"), this, {g: v});

var float32x4 = asmLink(asmCompile('glob', 'ffi', USE_ASM + F32 + "var g=f4(ffi.g); function f() {return f4(g)} return f"), this, {g: SIMD.float32x4(1,2,3,4)})();
assertEq(float32x4.x, 1);
assertEq(float32x4.y, 2);
assertEq(float32x4.z, 3);
assertEq(float32x4.w, 4);

for (var v of [1, {}, "totally legit SIMD variable", SIMD.int32x4(1,2,3,4)])
    assertCaught(asmCompile('glob', 'ffi', USE_ASM + F32 + "var g=f4(ffi.g); function f() {return f4(g)} return f"), this, {g: v});


var int32x4 = asmLink(asmCompile('glob', 'ffi', USE_ASM + I32 + "var g=i4(ffi.g); function f() {g=i4(4,5,6,7); return i4(g)} return f"), this, {g: SIMD.int32x4(1,2,3,4)})();
assertEq(int32x4.x, 4);
assertEq(int32x4.y, 5);
assertEq(int32x4.z, 6);
assertEq(int32x4.w, 7);

var float32x4 = asmLink(asmCompile('glob', 'ffi', USE_ASM + F32 + "var g=f4(ffi.g); function f() {g=f4(4.,5.,6.,7.); return f4(g)} return f"), this, {g: SIMD.float32x4(1,2,3,4)})();
assertEq(float32x4.x, 4);
assertEq(float32x4.y, 5);
assertEq(float32x4.z, 6);
assertEq(float32x4.w, 7);



assertAsmTypeFail('glob', USE_ASM + "var add = int32x4.add; return {}");
assertAsmTypeFail('glob', USE_ASM + I32A + I32 + "return {}");
assertAsmTypeFail('glob', USE_ASM + "var g = 3; var add = g.add; return {}");
assertAsmTypeFail('glob', USE_ASM + I32 + "var func = i4.doTheHarlemShake; return {}");
assertAsmTypeFail('glob', USE_ASM + I32 + "var div = i4.div; return {}");
assertAsmTypeFail('glob', USE_ASM + "var f32 = glob.Math.fround; var i4a = f32.add; return {}");


assertAsmLinkAlwaysFail(asmCompile('glob', USE_ASM + I32 + I32A + "function f() {} return f"), {});
assertAsmLinkAlwaysFail(asmCompile('glob', USE_ASM + I32 + I32A + "function f() {} return f"), {SIMD: Math.fround});

var oldInt32x4Add = SIMD.int32x4.add;
var code = asmCompile('glob', USE_ASM + I32 + I32A + "return {}");
for (var v of [42, Math.fround, SIMD.float32x4.add, function(){}, SIMD.int32x4.mul]) {
    SIMD.int32x4.add = v;
    assertAsmLinkFail(code, {SIMD: {int32x4: SIMD.int32x4}});
}
SIMD.int32x4.add = oldInt32x4Add; 
assertEq(asmLink(asmCompile('glob', USE_ASM + I32 + I32A + "function f() {} return f"), {SIMD: {int32x4: SIMD.int32x4}})(), undefined);



assertAsmTypeFail('glob', USE_ASM + I32 + I32A + "function f() {var x=i4(1,2,3,4); x=i4a();} return f");
assertAsmTypeFail('glob', USE_ASM + I32 + I32A + "function f() {var x=i4(1,2,3,4); x=i4a(x);} return f");
assertAsmTypeFail('glob', USE_ASM + I32 + I32A + "function f() {var x=i4(1,2,3,4); x=i4a(x, x, x);} return f");
assertAsmTypeFail('glob', USE_ASM + I32 + I32A + "function f() {var x=i4(1,2,3,4); x=i4a(13, 37);} return f");
assertAsmTypeFail('glob', USE_ASM + I32 + I32A + "function f() {var x=i4(1,2,3,4); x=i4a(23.10, 19.89);} return f");
assertAsmTypeFail('glob', USE_ASM + I32 + I32A + "function f() {var x=i4(1,2,3,4); x=i4a(x, 42);} return f");
assertAsmTypeFail('glob', USE_ASM + I32 + I32A + "function f() {var x=i4(1,2,3,4); x=i4a(x, 13.37);} return f");

assertAsmTypeFail('glob', USE_ASM + I32 + I32A + "function f() {var x=i4(1,2,3,4); var y=4; x=i4a(x, y);} return f");
assertAsmTypeFail('glob', USE_ASM + I32 + I32A + "function f() {var x=i4(0,0,0,0); var y=4; x=i4a(y, y);} return f");
assertAsmTypeFail('glob', USE_ASM + I32 + I32A + "function f() {var x=i4(0,0,0,0); var y=4; y=i4a(x, x);} return f");
assertAsmTypeFail('glob', USE_ASM + I32 + F32 + I32A + "function f() {var x=i4(0,0,0,0); var y=f4(4,3,2,1); x=i4a(x, y);} return f");
assertAsmTypeFail('glob', USE_ASM + I32 + F32 + I32A + "function f() {var x=i4(0,0,0,0); var y=f4(4,3,2,1); y=i4a(x, y);} return f");
assertAsmTypeFail('glob', USE_ASM + I32 + F32 + I32A + "function f() {var x=i4(0,0,0,0); var y=f4(4,3,2,1); y=i4a(x, x);} return f");
assertAsmTypeFail('glob', USE_ASM + I32 + F32 + F32A + "function f() {var x=i4(0,0,0,0); var y=f4(4,3,2,1); y=f4a(x, x);} return f");
assertAsmTypeFail('glob', USE_ASM + I32 + F32 + F32A + "function f() {var x=i4(0,0,0,0); var y=f4(4,3,2,1); y=f4a(x, y);} return f");
assertAsmTypeFail('glob', USE_ASM + I32 + F32 + F32A + "function f() {var x=i4(0,0,0,0); var y=f4(4,3,2,1); x=f4a(y, y);} return f");

assertAsmTypeFail('glob', USE_ASM + I32 + I32A + 'function f() {var x=i4(1,2,3,4); var y=0; y=i4(x,x)|0} return f');
assertAsmTypeFail('glob', USE_ASM + I32 + I32A + 'function f() {var x=i4(1,2,3,4); var y=0.; y=+i4(x,x)} return f');

CheckI4(I32A, 'var z=i4(1,2,3,4); var y=i4(0,1,0,3); var x=i4(0,0,0,0); x=i4a(z,y)', [1,3,3,7]);
CheckI4(I32A, 'var x=i4(2,3,4,5); var y=i4(0,1,0,3); x=i4a(x,y)', [2,4,4,8]);
CheckI4(I32A, 'var x=i4(1,2,3,4); x=i4a(x,x)', [2,4,6,8]);
CheckI4(I32A, 'var x=i4(' + INT32_MAX + ',2,3,4); var y=i4(1,1,0,3); x=i4a(x,y)', [INT32_MIN,3,3,7]);
CheckI4(I32A, 'var x=i4(' + INT32_MAX + ',2,3,4); var y=i4(1,1,0,3); x=i4(i4a(x,y))', [INT32_MIN,3,3,7]);

CheckF4(F32A, 'var x=f4(1,2,3,4); x=f4a(x,x)', [2,4,6,8]);
CheckF4(F32A, 'var x=f4(1,2,3,4); var y=f4(4,3,5,2); x=f4a(x,y)', [5,5,8,6]);
CheckF4(F32A, 'var x=f4(13.37,2,3,4); var y=f4(4,3,5,2); x=f4a(x,y)', [Math.fround(13.37) + 4,5,8,6]);
CheckF4(F32A, 'var x=f4(13.37,2,3,4); var y=f4(4,3,5,2); x=f4(f4a(x,y))', [Math.fround(13.37) + 4,5,8,6]);


CheckI4(I32S, 'var x=i4(1,2,3,4); var y=i4(-1,1,0,2); x=i4s(x,y)', [2,1,3,2]);
CheckI4(I32S, 'var x=i4(5,4,3,2); var y=i4(1,2,3,4); x=i4s(x,y)', [4,2,0,-2]);
CheckI4(I32S, 'var x=i4(1,2,3,4); x=i4s(x,x)', [0,0,0,0]);
CheckI4(I32S, 'var x=i4(' + INT32_MIN + ',2,3,4); var y=i4(1,1,0,3); x=i4s(x,y)', [INT32_MAX,1,3,1]);
CheckI4(I32S, 'var x=i4(' + INT32_MIN + ',2,3,4); var y=i4(1,1,0,3); x=i4(i4s(x,y))', [INT32_MAX,1,3,1]);

CheckF4(F32S, 'var x=f4(1,2,3,4); x=f4s(x,x)', [0,0,0,0]);
CheckF4(F32S, 'var x=f4(1,2,3,4); var y=f4(4,3,5,2); x=f4s(x,y)', [-3,-1,-2,2]);
CheckF4(F32S, 'var x=f4(13.37,2,3,4); var y=f4(4,3,5,2); x=f4s(x,y)', [Math.fround(13.37) - 4,-1,-2,2]);
CheckF4(F32S, 'var x=f4(13.37,2,3,4); var y=f4(4,3,5,2); x=f4(f4s(x,y))', [Math.fround(13.37) - 4,-1,-2,2]);


assertAsmTypeFail('glob', USE_ASM + I32 + "var f4m=i4.mul; function f() {} return f");
assertAsmTypeFail('glob', USE_ASM + I32 + "var f4d=i4.div; function f() {} return f");

CheckF4(F32M, 'var x=f4(1,2,3,4); x=f4m(x,x)', [1,4,9,16]);
CheckF4(F32M, 'var x=f4(1,2,3,4); var y=f4(4,3,5,2); x=f4m(x,y)', [4,6,15,8]);
CheckF4(F32M, 'var x=f4(13.37,2,3,4); var y=f4(4,3,5,2); x=f4m(x,y)', [Math.fround(13.37) * 4,6,15,8]);
CheckF4(F32M, 'var x=f4(13.37,2,3,4); var y=f4(4,3,5,2); x=f4(f4m(x,y))', [Math.fround(13.37) * 4,6,15,8]);


var f32x4 = SIMD.float32x4(0, NaN, -0, NaN);
var another = SIMD.float32x4(NaN, -1, -0, NaN);
assertEqX4(asmLink(asmCompile('glob', USE_ASM + F32 + F32M + "function f(x, y) {x=f4(x); y=f4(y); x=f4m(x,y); return f4(x);} return f"), this)(f32x4, another), [NaN, NaN, 0, NaN]);

CheckF4(F32D, 'var x=f4(1,2,3,4); x=f4d(x,x)', [1,1,1,1]);
CheckF4(F32D, 'var x=f4(1,2,3,4); var y=f4(4,3,5,2); x=f4d(x,y)', [1/4,2/3,3/5,2]);
CheckF4(F32D, 'var x=f4(13.37,1,1,4); var y=f4(4,0,-0.,2); x=f4d(x,y)', [Math.fround(13.37) / 4,+Infinity,-Infinity,2]);


var f32x4 = SIMD.float32x4(0, 0, -0, NaN);
var another = SIMD.float32x4(0, -0, 0, 0);
assertEqX4(asmLink(asmCompile('glob', USE_ASM + F32 + F32D + "function f(x,y) {x=f4(x); y=f4(y); x=f4d(x,y); return f4(x);} return f"), this)(f32x4, another), [NaN, NaN, NaN, NaN]);


assertEqX4(asmLink(asmCompile('glob', USE_ASM + I32 + 'function f(){var x=i4(1,2,3,4); return i4(x); x=i4(5,6,7,8); return i4(x);} return f'), this)(), [1, 2, 3, 4]);
assertEqX4(asmLink(asmCompile('glob', USE_ASM + I32 + 'function f(){var x=i4(1,2,3,4); var c=0; return i4(x); c=x.x|0; return i4(x);} return f'), this)(), [1, 2, 3, 4]);
assertEqX4(asmLink(asmCompile('glob', USE_ASM + I32 + I32A + 'function f(){var x=i4(1,2,3,4); var c=0; return i4(x); x=i4a(x,x); return i4(x);} return f'), this)(), [1, 2, 3, 4]);
assertEqX4(asmLink(asmCompile('glob', USE_ASM + I32 + I32S + 'function f(){var x=i4(1,2,3,4); var c=0; return i4(x); x=i4s(x,x); return i4(x);} return f'), this)(), [1, 2, 3, 4]);



assertAsmTypeFail('glob', USE_ASM + I32 + "var fround=glob.Math.fround; function f() {var x=i4(1,2,3,4); return +fround(x);} return f");
assertAsmTypeFail('glob', USE_ASM + I32 + "var sin=glob.Math.sin; function f() {var x=i4(1,2,3,4); return +sin(x);} return f");
assertAsmTypeFail('glob', USE_ASM + I32 + "var ceil=glob.Math.ceil; function f() {var x=i4(1,2,3,4); return +ceil(x);} return f");
assertAsmTypeFail('glob', USE_ASM + I32 + "var pow=glob.Math.pow; function f() {var x=i4(1,2,3,4); return +pow(1.0, x);} return f");

assertAsmTypeFail('glob', USE_ASM + I32 + "var fround=glob.Math.fround; function f() {var x=i4(1,2,3,4); x=i4(fround(3));} return f");
assertAsmTypeFail('glob', USE_ASM + I32 + "var sin=glob.Math.sin; function f() {var x=i4(1,2,3,4); x=i4(sin(3.0));} return f");
assertAsmTypeFail('glob', USE_ASM + I32 + "var ceil=glob.Math.sin; function f() {var x=i4(1,2,3,4); x=i4(ceil(3.0));} return f");
assertAsmTypeFail('glob', USE_ASM + I32 + "var pow=glob.Math.pow; function f() {var x=i4(1,2,3,4); x=i4(pow(1.0, 2.0));} return f");



assertAsmTypeFail('glob', 'ffi', USE_ASM + I32 + "var func=ffi.func; function f() {var x=i4(1,2,3,4); func(x);} return f");
assertAsmTypeFail('glob', 'ffi', USE_ASM + F32 + "var func=ffi.func; function f() {var x=f4(1,2,3,4); func(x);} return f");


assertAsmTypeFail('glob', 'ffi', USE_ASM + I32 + "var func=ffi.func; function f() {var x=i4(1,2,3,4); x=i4(func());} return f");
assertAsmTypeFail('glob', 'ffi', USE_ASM + F32 + "var func=ffi.func; function f() {var x=f4(1,2,3,4); x=f4(func());} return f");




var code = USE_ASM + I32 + I32A + `
    var check = ffi.check;

    function g() {
        var i = 0;
        var y = i4(0,0,0,0);
        var tmp = i4(0,0,0,0); var z = i4(1,1,1,1);
        var w = i4(5,5,5,5);
        for (; (i|0) < 30; i = i + 1 |0)
            y = i4a(z, y);
        y = i4a(w, y);
        check(y.x | 0, y.y | 0, y.z | 0, y.w | 0);
        return i4(y);
    }

    function f(x) {
        x = i4(x);
        var y = i4(0,0,0,0);
        y = i4(g());
        check(y.x | 0, y.y | 0, y.z | 0, y.w | 0);
        return i4(x);
    }
    return f;
`;

var v4 = SIMD.int32x4(1,2,3,4);
function check(x, y, z, w) {
    assertEq(x, 35);
    assertEq(y, 35);
    assertEq(z, 35);
    assertEq(w, 35);
}
var ffi = {check};
assertEqX4(asmLink(asmCompile('glob', 'ffi', code), this, ffi)(v4), [1,2,3,4]);



var code = USE_ASM + I32 + I32A + `
    var assertEq = ffi.assertEq;

    function internal([args]) {
        [coerc]
        assertEq([last].x | 0, [i] | 0);
        assertEq([last].y | 0, [i] + 1 |0);
        assertEq([last].z | 0, [i] + 2 |0);
        assertEq([last].w | 0, [i] + 3 |0);
    }

    function external() {
        [decls]
        internal([args]);
    }
    return external;
`;

var ffi = {assertEq};
var args = '';
var decls = '';
var coerc = '';
for (var i = 1; i < 10; ++i) {
    var j = i;
    args += ((i > 1) ? ', ':'') + 'x' + i;
    decls += 'var x' + i + ' = i4(' + j++ + ', ' + j++ + ', ' + j++ + ', ' + j++ + ');\n';
    coerc += 'x' + i + ' = i4(x' + i + ');\n';
    last = 'x' + i;
    var c = code.replace(/\[args\]/g, args)
                .replace(/\[last\]/g, last)
                .replace(/\[decls\]/i, decls)
                .replace(/\[coerc\]/i, coerc)
                .replace(/\[i\]/g, i);
    asmLink(asmCompile('glob', 'ffi', c), this, ffi)();
}


var code = `
    "use asm";
    var i4 = glob.SIMD.int32x4;
    var i4a = i4.add;
    var assertEq = ffi.assertEq;
    function g() {
        var x = i4(1,2,3,4);
        var y = i4(2,3,4,5);
        var z = i4(0,0,0,0);
        z = i4a(x, y);
        assertEq(z.x | 0, 3);
        assertEq(z.y | 0, 5);
        assertEq(z.z | 0, 7);
        assertEq(z.w | 0, 9);
    }
    return g
`
asmLink(asmCompile('glob', 'ffi', code), this, assertEqFFI)();

(function() {
    var code = `
        "use asm";
        var i4 = glob.SIMD.int32x4;
        var i4a = i4.add;
        var assertEq = ffi.assertEq;
        var one = ffi.one;

        // Function call with arguments on the stack (1 on x64, 3 on x86)
        function h(x1, x2, x3, x4, x5, x6, x7) {
            x1=x1|0
            x2=x2|0
            x3=x3|0
            x4=x4|0
            x5=x5|0
            x6=x6|0
            x7=x7|0
            return x1 + x2 |0
        }

        function g() {
            var x = i4(1,2,3,4);
            var y = i4(2,3,4,5);
            var z = i4(0,0,0,0);
            var w = 1;
            z = i4a(x, y);
            w = w + (one() | 0) | 0;
            assertEq(z.x | 0, 3);
            assertEq(z.y | 0, 5);
            assertEq(z.z | 0, 7);
            assertEq(z.w | 0, 9);
            h(1, 2, 3, 4, 42, 42, 42)|0
            return w | 0;
        }
        return g
    `;

    asmLink(asmCompile('glob', 'ffi', code), this, {assertEq: assertEq, one: () => 1})();
})();



(function() {
    var code = `
        "use asm";
        var i4 = glob.SIMD.int32x4;
        function h(
            // In registers:
            gpr1, gpr2, gpr3, gpr4, xmm1, xmm2, xmm3, xmm4, xmm5, xmm6, xmm7, xmm8,
            // On the stack:
            sint1, ssimd1, sdouble1, ssimd2, sint2, sint3, sint4, ssimd3, sdouble2
            )
        {
            gpr1=gpr1|0;
            gpr2=gpr2|0;
            gpr3=gpr3|0;
            gpr4=gpr4|0;

            xmm1=+xmm1;
            xmm2=+xmm2;
            xmm3=+xmm3;
            xmm4=+xmm4;
            xmm5=+xmm5;
            xmm6=+xmm6;
            xmm7=+xmm7;
            xmm8=+xmm8;

            sint1=sint1|0;
            ssimd1=i4(ssimd1);
            sdouble1=+sdouble1;
            ssimd2=i4(ssimd2);
            sint2=sint2|0;
            sint3=sint3|0;
            sint4=sint4|0;
            ssimd3=i4(ssimd3);
            sdouble2=+sdouble2;

            return (ssimd1.x|0) + (ssimd2.y|0) + (ssimd3.z|0) + sint2 + gpr3 | 0;
        }

        function g() {
            var simd1 = i4(1,2,3,4);
            var simd2 = i4(5,6,7,8);
            var simd3 = i4(9,10,11,12);
            return h(1, 2, 3, 4,
                     1., 2., 3., 4., 5., 6., 7., 8.,
                     5, simd1, 9., simd2, 6, 7, 8, simd3, 10.) | 0;
        }
        return g
    `;

    assertEq(asmLink(asmCompile('glob', 'ffi', code), this)(), 1 + 6 + 11 + 6 + 3);
})();





(function() {
    var iters = 2000000;
    var code = `
    "use asm";
    var i4 = glob.SIMD.int32x4;
    var i4a = i4.add;
    function _() {
        var i = 0;
        var n = i4(0,0,0,0);
        var one = i4(1,1,1,1);
        for (; (i>>>0) < ` + iters + `; i=(i+1)>>>0) {
            n = i4a(n, one);
        }
        return i4(n);
    }
    return _;`;
    
    
    timeout(1000);
    var x4 = asmLink(asmCompile('glob', code), this)();
    assertEq(x4.x, iters);
    assertEq(x4.y, iters);
    assertEq(x4.z, iters);
    assertEq(x4.w, iters);
})();

} catch(e) {
    print('Stack:', e.stack)
    print('Error:', e)
    throw e;
}

