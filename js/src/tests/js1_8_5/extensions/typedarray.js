








var BUGNUMBER = 532774;
var summary = 'js typed arrays (webgl arrays)';
var actual = '';
var expect = '';


test();


function test()
{
    enterFunc ('test');
    printBugNumber(BUGNUMBER);
    printStatus(summary);
    
    var TestPassCount = 0;
    var TestFailCount = 0;
    var TestTodoCount = 0;

    var TODO = 1;

    function check(fun, todo) {
        var thrown = null;
        var success = false;
        try {
            success = fun();
        } catch (x) {
            thrown = x;
        }

        if (thrown)
            success = false;

        if (todo) {
            TestTodoCount++;

            if (success) {
                var ex = new Error;
                print ("=== TODO but PASSED? ===");
                print (ex.stack);
                print ("========================");
            }

            return;
        }

        if (success) {
            TestPassCount++;
        } else {
            TestFailCount++;

            var ex = new Error;
            print ("=== FAILED ===");
            print (ex.stack);
            if (thrown) {
                print ("    threw exception:");
                print (thrown);
            }
            print ("==============");
        }
    }

    function checkThrows(fun, todo) {
        let thrown = false;
        try {
            fun();
        } catch (x) {
            thrown = true;
        }

        check(function() thrown, todo);
    }

    var buf, buf2;

    buf = new ArrayBuffer(100);
    check(function() buf);
    check(function() buf.byteLength == 100);

    buf.byteLength = 50;
    check(function() buf.byteLength == 100);

    var zerobuf = new ArrayBuffer(0);
    check(function() zerobuf);
    check(function() zerobuf.byteLength == 0);

    check(function() (new Int32Array(zerobuf)).length == 0);
    checkThrows(function() new Int32Array(zerobuf, 1));

    var zerobuf2 = new ArrayBuffer();
    check(function() zerobuf2.byteLength == 0);

    checkThrows(function() new ArrayBuffer(-100));
    
    checkThrows(function() new ArrayBuffer("abc"), TODO);

    var zeroarray = new Int32Array(0);
    check(function() zeroarray.length == 0);
    check(function() zeroarray.byteLength == 0);
    check(function() zeroarray.buffer);
    check(function() zeroarray.buffer.byteLength == 0);

    var zeroarray2 = new Int32Array();
    check(function() zeroarray2.length == 0);
    check(function() zeroarray2.byteLength == 0);
    check(function() zeroarray2.buffer);
    check(function() zeroarray2.buffer.byteLength == 0);

    var a = new Int32Array(20);
    check(function() a);
    check(function() a.length == 20);
    check(function() a.byteLength == 80);
    check(function() a.byteOffset == 0);
    check(function() a.buffer);
    check(function() a.buffer.byteLength == 80);

    var b = new Uint8Array(a.buffer, 4, 4);
    check(function() b);
    check(function() b.length == 4);
    check(function() b.byteLength == 4);
    check(function() a.buffer == b.buffer);

    b[0] = 0xaa;
    b[1] = 0xbb;
    b[2] = 0xcc;
    b[3] = 0xdd;

    check(function() a[0] == 0);
    check(function() a[1] != 0);
    check(function() a[2] == 0);

    buf = new ArrayBuffer(4);
    check(function() (new Int8Array(buf)).length == 4);
    check(function() (new Uint8Array(buf)).length == 4);
    check(function() (new Int16Array(buf)).length == 2);
    check(function() (new Uint16Array(buf)).length == 2);
    check(function() (new Int32Array(buf)).length == 1);
    check(function() (new Uint32Array(buf)).length == 1);
    check(function() (new Float32Array(buf)).length == 1);
    checkThrows(function() (new Float64Array(buf)));
    buf2 = new ArrayBuffer(8);
    check(function() (new Float64Array(buf2)).length == 1);

    buf = new ArrayBuffer(5);
    check(function() buf);
    check(function() buf.byteLength == 5);

    check(function() new Int32Array(buf, 0, 1));
    checkThrows(function() new Int32Array(buf, 0));
    check(function() new Int8Array(buf, 0));

    check(function() (new Int8Array(buf, 3)).byteLength == 2);
    checkThrows(function() new Int8Array(buf, 500));
    checkThrows(function() new Int8Array(buf, 0, 50));
    checkThrows(function() new Float32Array(buf, 500));
    checkThrows(function() new Float32Array(buf, 0, 50));

    var sl = a.subarray(5,10);
    check(function() sl.length == 5);
    check(function() sl.buffer == a.buffer);
    check(function() sl.byteLength == 20);
    check(function() sl.byteOffset == 20);

    check(function() a.subarray(5,5).length == 0);
    check(function() a.subarray(-5).length == 5);
    check(function() a.subarray(-100).length == 20);
    check(function() a.subarray(0, 2).length == 2);
    check(function() a.subarray().length == a.length);
    check(function() a.subarray(-7,-5).length == 2);
    check(function() a.subarray(-5,-7).length == 0);
    check(function() a.subarray(15).length == 5);

    a = new Uint8Array([0xaa, 0xbb, 0xcc]);
    check(function() a.length == 3);
    check(function() a.byteLength == 3);
    check(function() a[1] == 0xbb);

    
    checkThrows(function() new Int32Array([0xaa, "foo", 0xbb]), TODO);

    checkThrows(function() new Int32Array(-100));

    a = new Uint8Array(3);
    
    
    
    
    check(function() (a[0] = "10") && (a[0] == 10));

    
    a = new Uint8ClampedArray(4);
    a[0] = 128;
    a[1] = 512;
    a[2] = -123.723;
    a[3] = "foopy";

    check(function() a[0] == 128);
    check(function() a[1] == 255);
    check(function() a[2] == 0);
    check(function() a[3] == 0);

    
    var x = Array(5);
    x[0] = "hello";
    x[1] = { };
    
    x[3] = undefined;
    x[4] = true;

    a = new Uint8Array(x);
    check(function() a[0] == 0);
    check(function() a[1] == 0);
    check(function() a[2] == 0);
    check(function() a[3] == 0);
    check(function() a[4] == 1);

    a = new Float32Array(x);
    check(function() !(a[0] == a[0]));
    check(function() !(a[1] == a[1]));
    check(function() !(a[2] == a[2]));
    check(function() !(a[3] == a[3]));
    check(function() a[4] == 1);

    
    var empty = new Int32Array(0);
    a = new Int32Array(9);

    empty.set([]);
    empty.set([], 0);
    empty.set(empty);

    checkThrows(function() empty.set([1]));
    checkThrows(function() empty.set([1], 0));
    checkThrows(function() empty.set([1], 1));

    a.set([]);
    a.set([], 3);
    a.set([], 9);
    a.set(a);

    a.set(empty);
    a.set(empty, 3);
    a.set(empty, 9);
    a.set(Array.prototype);
    checkThrows(function() a.set(empty, 100));

    checkThrows(function() a.set([1,2,3,4,5,6,7,8,9,10]));
    checkThrows(function() a.set([1,2,3,4,5,6,7,8,9,10], 0));
    checkThrows(function() a.set([1,2,3,4,5,6,7,8,9,10], 0x7fffffff));
    checkThrows(function() a.set([1,2,3,4,5,6,7,8,9,10], 0xffffffff));
    checkThrows(function() a.set([1,2,3,4,5,6], 6));

    checkThrows(function() a.set(new Array(0x7fffffff)));
    checkThrows(function() a.set([1,2,3], 2147483647));

    a.set(ArrayBuffer.prototype);
    a.set(Int16Array.prototype);
    a.set(Int32Array.prototype);

    a.set([1,2,3]);
    a.set([4,5,6], 3);
    check(function()
          a[0] == 1 && a[1] == 2 && a[2] == 3 &&
          a[3] == 4 && a[4] == 5 && a[5] == 6 &&
          a[6] == 0 && a[7] == 0 && a[8] == 0);

    b = new Float32Array([7,8,9]);
    a.set(b, 0);
    a.set(b, 3);
    check(function()
          a[0] == 7 && a[1] == 8 && a[2] == 9 &&
          a[3] == 7 && a[4] == 8 && a[5] == 9 &&
          a[6] == 0 && a[7] == 0 && a[8] == 0);
    a.set(a.subarray(0,3), 6);
    check(function()
          a[0] == 7 && a[1] == 8 && a[2] == 9 &&
          a[3] == 7 && a[4] == 8 && a[5] == 9 &&
          a[6] == 7 && a[7] == 8 && a[8] == 9);

    a.set([1,2,3,4,5,6,7,8,9]);
    a.set(a.subarray(0,6), 3);
    check(function()
          a[0] == 1 && a[1] == 2 && a[2] == 3 &&
          a[3] == 1 && a[4] == 2 && a[5] == 3 &&
          a[6] == 4 && a[7] == 5 && a[8] == 6);

    a.set(a.subarray(3,9), 0);
    check(function()
          a[0] == 1 && a[1] == 2 && a[2] == 3 &&
          a[3] == 4 && a[4] == 5 && a[5] == 6 &&
          a[6] == 4 && a[7] == 5 && a[8] == 6);

    
    
    a.subarray(0,3).set(a.subarray(3,6), 0);
    check(function()
          a[0] == 4 && a[1] == 5 && a[2] == 6 &&
          a[3] == 4 && a[4] == 5 && a[5] == 6 &&
          a[6] == 4 && a[7] == 5 && a[8] == 6);

    a = new ArrayBuffer(0x10);
    checkThrows(function() new Uint32Array(buffer, 4, 0x3FFFFFFF));

    checkThrows(function() new Float32Array(null));

    a = new Uint8Array(0x100);
    checkThrows(function() Uint32Array.prototype.subarray.apply(a, [0, 0x100]));

    
    
    check(function() new Int32Array(ArrayBuffer.prototype).length == 0);
    check(function() new Int32Array(Int32Array.prototype).length == 0);
    check(function() new Int32Array(Float64Array.prototype).length == 0);

    
    
    
    

    check(function() Int32Array.BYTES_PER_ELEMENT == 4);
    check(function() (new Int32Array(4)).BYTES_PER_ELEMENT == 4);
    check(function() (new Int32Array()).BYTES_PER_ELEMENT == 4);
    check(function() (new Int32Array(0)).BYTES_PER_ELEMENT == 4);
    check(function() Int16Array.BYTES_PER_ELEMENT == Uint16Array.BYTES_PER_ELEMENT);

    
    
    check(function() (new Float32Array(Math.sqrt(4))).length == 2);
    check(function() (new Float32Array({ length: 10 })).length == 10);
    check(function() (new Float32Array({})).length == 0);
    checkThrows(function() new Float32Array("3"));
    checkThrows(function() new Float32Array(null));
    checkThrows(function() new Float32Array(undefined));

    
    check(function() (new Int32Array([NaN])[0]) == 0);
    check(function() { var q = new Float32Array([NaN])[0]; return q != q; });

    
    
    
    
    buf = new ArrayBuffer(128);
    a = new Uint32Array(buf, 0, 4);
    check(function() a[0] ==  0 && a[1] == 0 && a[2] == 0 && a[3] == 0);
    buf.a = 42;
    buf.b = "abcdefgh";
    buf.c = {a:'literal'};
    check(function() a[0] ==  0 && a[1] == 0 && a[2] == 0 && a[3] == 0);

    check(function() buf.a == 42);
    delete buf.a;
    check(function() !buf.a);

    
    
    a = new Uint8Array(120);
    check(function() a.byteLength == 120);
    check(function() a.length == 120);
    for (var i = 0; i < a.length; i++)
        check(function() a[i] == 0)

    a = new Uint8Array(121);
    check(function() a.byteLength == 121);
    check(function() a.length == 121);
    for (var i = 0; i < a.length; i++)
        check(function() a[i] == 0)
    print ("done");

    reportCompare(0, TestFailCount, "typed array tests");

    exitFunc ('test');
}
