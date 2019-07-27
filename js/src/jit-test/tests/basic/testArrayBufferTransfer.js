load(libdir + "asserts.js");



if (!ArrayBuffer.transfer)
    quit();

var XF = ArrayBuffer.transfer;

assertEq(typeof XF, "function");
assertEq(XF.length, 2);


assertThrowsInstanceOf(()=>XF(), Error);
assertThrowsInstanceOf(()=>XF(undefined), Error);
assertThrowsInstanceOf(()=>XF(null), Error);
assertThrowsInstanceOf(()=>XF({}), Error);
assertThrowsInstanceOf(()=>XF(new Int32Array(1)), Error);
var buf = new ArrayBuffer(1);
neuter(buf, 'change-data');
assertThrowsInstanceOf(()=>XF(buf), TypeError);


var buf = new ArrayBuffer(1);
assertThrowsInstanceOf(()=>XF(buf, -1), Error);
assertThrowsInstanceOf(()=>XF(buf, {valueOf() { return -1 }}), Error);
assertThrowsInstanceOf(()=>XF(buf, {toString() { return "-1" }}), Error);
assertThrowsValue(()=>XF(buf, {valueOf() { throw "wee" }}), "wee");


var buf = new ArrayBuffer(1);
assertThrowsInstanceOf(()=>XF(buf, Math.pow(2,31)), Error);
buf = XF(buf, Math.pow(2,32));
assertEq(buf.byteLength, 0);
buf = XF(buf, Math.pow(2,32) + 10);
assertEq(buf.byteLength, 10);


var buf1 = new ArrayBuffer(0);
var buf2 = XF(buf1);
assertEq(buf1.byteLength, 0);
assertEq(buf2.byteLength, 0);
assertThrowsInstanceOf(()=>XF(buf1), TypeError);

var buf1 = new ArrayBuffer(3);
var buf2 = XF(buf1);
assertEq(buf1.byteLength, 0);
assertEq(buf2.byteLength, 3);
assertThrowsInstanceOf(()=>XF(buf1), TypeError);

var buf1 = new ArrayBuffer(9);
var buf2 = XF(buf1, undefined);
assertEq(buf1.byteLength, 0);
assertEq(buf2.byteLength, 9);
assertThrowsInstanceOf(()=>XF(buf1), TypeError);


function test(N1, N2) {
    var buf1 = new ArrayBuffer(N1);
    var i32 = new Int32Array(buf1);
    for (var i = 0; i < i32.length; i++)
        i32[i] = i;

    var buf2 = XF(buf1, N2);

    assertEq(buf1.byteLength, 0);
    assertEq(i32.length, 0);
    assertEq(buf2.byteLength, N2);
    var i32 = new Int32Array(buf2);
    for (var i = 0; i < Math.min(N1, N2)/4; i++)
        assertEq(i32[i], i);
    for (var i = Math.min(N1, N2)/4; i < i32.length; i++) {
        assertEq(i32[i], 0);
        i32[i] = -i;
    }
}
test(0, 0);
test(0, 4);
test(4, 0);
test(4, 4);
test(0, 1000);
test(4, 1000);
test(1000, 0);
test(1000, 4);
test(1000, 1000);
