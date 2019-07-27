









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

    function check(fun, msg, todo) {
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
            if (msg)
                print (msg);
            print (ex.stack);
            if (thrown) {
                print ("    threw exception:");
                print (thrown);
            }
            print ("==============");
        }
    }

    function checkThrows(fun, type, todo) {
        var thrown = false;
        try {
            fun();
        } catch (x) {
            thrown = x;
        }

        if (typeof(type) !== 'undefined')
            if (thrown) {
                check(function () thrown instanceof type,
                      "expected " + type.name + " but saw " + thrown,
                      todo);
            } else {
                check(function () thrown, "expected " + type.name + " but no exception thrown", todo);
            }
        else
            check(function () thrown, undefined, todo);
    }

    function checkThrowsTODO(fun, type) {
        checkThrows(fun, type, true);
    }

    function testBufferManagement() {
        
        var buffer = new ArrayBuffer(128);
        buffer = null;
        gc();

        
        buffer = new ArrayBuffer(128);
        var v1 = new Uint8Array(buffer);
        gc();
        v1 = null;
        gc();
        buffer = null;
        gc();

        
        buffer = new ArrayBuffer(128);
        v1 = new Uint8Array(buffer);
        gc();
        buffer = null;
        gc();
        v1 = null;
        gc();

        
        buffer = new ArrayBuffer(128);
        v1 = new Uint8Array(buffer);
        v2 = new Uint8Array(buffer);
        gc();
        v1 = null;
        gc();
        v2 = null;
        gc();

        
        buffer = new ArrayBuffer(128);
        v1 = new Uint8Array(buffer);
        v2 = new Uint8Array(buffer);
        gc();
        v2 = null;
        gc();
        v1 = null;
        gc();

        
        buffer = new ArrayBuffer(128);
        for (let order = 0; order < 16; order++) {
            var views = [ new Uint8Array(buffer),
                          new Uint8Array(buffer),
                          new Uint8Array(buffer),
                          new Uint8Array(buffer) ];
            gc();

            
            for (let i = 0; i < 4; i++) {
                if (order & (1 << i))
                    views[i] = null;
            }

            gc();

            views = null;
            gc();
        }

        
        buffer = new ArrayBuffer(128);
        for (let order = 0; order < 4*3*2*1; order++) {
            var views = [ new Uint8Array(buffer),
                          new Uint8Array(buffer),
                          new Uint8Array(buffer),
                          new Uint8Array(buffer) ];
            gc();

            var sequence = [ 0, 1, 2, 3 ];
            let groupsize = 4*3*2*1;
            let o = order;
            for (let i = 4; i > 0; i--) {
                groupsize = groupsize / i;
                let which = Math.floor(o/groupsize);
                [ sequence[i-1], sequence[which] ] = [ sequence[which], sequence[i-1] ];
                o = o % groupsize;
            }

            for (let i = 0; i < 4; i++) {
                views[i] = null;
                gc();
            }
        }

        
        var views = [];
        for (let numViews of [ 1, 2, 0, 3, 2, 1 ]) {
            buffer = new ArrayBuffer(128);
            for (let viewNum = 0; viewNum < numViews; viewNum++) {
                views.push(new Int8Array(buffer));
            }
        }

        gcparam('markStackLimit', 200);
        var forceOverflow = [ buffer ];
        for (let i = 0; i < 1000; i++) {
            forceOverflow = [ forceOverflow ];
        }
        gc();
        buffer = null;
        views = null;
        gcslice(2); gcslice(2); gcslice(2); gcslice(2); gcslice(2); gcslice(2); gc();
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

    checkThrows(function() new ArrayBuffer(-100), RangeError);
    
    checkThrowsTODO(function() new ArrayBuffer("abc"), TypeError);

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

    
    checkThrowsTODO(function() new Int32Array([0xaa, "foo", 0xbb]), Error);

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
    checkThrows(function () a.set(Int16Array.prototype), TypeError);
    checkThrows(function () a.set(Int32Array.prototype), TypeError);

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
    b = Uint32Array.prototype.subarray.apply(a, [0, 0x100]);
    check(() => Object.prototype.toString.call(b) === "[object Uint8Array]");
    check(() => b.buffer === a.buffer);
    check(() => b.length === a.length);
    check(() => b.byteLength === a.byteLength);
    check(() => b.byteOffset === a.byteOffset);
    check(() => b.BYTES_PER_ELEMENT === a.BYTES_PER_ELEMENT);

    
    
    
    checkThrows(function() ArrayBuffer.prototype.byteLength, TypeError);
    checkThrows(function() Int32Array.prototype.length, TypeError);
    checkThrows(function() Int32Array.prototype.byteLength, TypeError);
    checkThrows(function() Int32Array.prototype.byteOffset, TypeError);
    checkThrows(function() Float64Array.prototype.length, TypeError);
    checkThrows(function() Float64Array.prototype.byteLength, TypeError);
    checkThrows(function() Float64Array.prototype.byteOffset, TypeError);

    
    
    
    check(function() Int32Array.prototype.length = true);
    check(function() Float64Array.prototype.length = true);
    check(function() Int32Array.prototype.byteLength = true);
    check(function() Float64Array.prototype.byteLength = true);
    check(function() Int32Array.prototype.byteOffset = true);
    check(function() Float64Array.prototype.byteOffset = true);

    
    
    check(function() (new Int32Array(ArrayBuffer)).length >= 0);
    check(function() (new Int32Array(Int32Array)).length >= 0);
    check(function() (new Int32Array(Float64Array)).length >= 0);

    
    
    
    
    
    
    
    
    
    

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

    
    a = new Uint8Array(100);
    a[99] = 5;
    b = new Uint8Array(a.buffer, 9); 
    
    for (var i = 0; i < b.length; i++)
        check(function() b[90] == 5)

    
    var alien = newGlobal();

    var alien_view = alien.eval('view = new Uint8Array(7)');
    var alien_buffer = alien.eval('buffer = view.buffer');

    
    
    
    
    var view = new Int8Array(alien_buffer);

    
    alien_view[3] = 77;
    check(function () view[3] == 77);

    
    check(function () isProxy(alien_view));
    check(function () isProxy(alien_buffer));
    check(function () isProxy(view)); 

    
    check(function () alien_buffer.byteLength == 7);
    check(function () alien_view.byteLength == 7);
    check(function () view.byteLength == 7);

    
    simple = new Int8Array(12);
    check(function () Object.getPrototypeOf(view) == Object.getPrototypeOf(simple));
    check(function () Object.getPrototypeOf(view) == Int8Array.prototype);

    
    check(() => !simple.hasOwnProperty('byteLength'));
    check(() => !Int8Array.prototype.hasOwnProperty('byteLength'));
    check(() => Object.getPrototypeOf(Int8Array.prototype).hasOwnProperty('byteLength'));

    check(() => !simple.hasOwnProperty("BYTES_PER_ELEMENT"));
    check(() => Int8Array.prototype.hasOwnProperty("BYTES_PER_ELEMENT"));
    check(() => !Object.getPrototypeOf(Int8Array.prototype).hasOwnProperty("BYTES_PER_ELEMENT"));

    
    
    
    if (false) {
        check(function () simple.byteLength == 12);
        getter = Object.getOwnPropertyDescriptor(Int8Array.prototype, 'byteLength').get;
        Object.defineProperty(Int8Array.prototype, 'byteLength', { get: function () { return 1 + getter.apply(this) } });
        check(function () simple.byteLength == 13);
    }

    
    var numbers = [ 0, 1, 2, 3, 4, 5, 6, 7, 8 ];

    function tastring(tarray) {
        return [ x for (x of tarray) ].toString();
    }

    function checkCopyWithin(offset, start, end, dest, want) {
        var numbers_buffer = new Uint8Array(numbers).buffer;
        var view = new Int8Array(numbers_buffer, offset);
        view.copyWithin(dest, start, end);
        check(function () tastring(view) == want.toString());
        if (tastring(view) != want.toString()) {
            print("Wanted: " + want.toString());
            print("Got   : " + tastring(view));
        }
    }

    
    checkCopyWithin(0, 2, 5, 4, [ 0, 1, 2, 3, 2, 3, 4, 7, 8 ]);

    
    checkCopyWithin(0, -7,  5,  4, [ 0, 1, 2, 3, 2, 3, 4, 7, 8 ]);
    checkCopyWithin(0,  2, -4,  4, [ 0, 1, 2, 3, 2, 3, 4, 7, 8 ]);
    checkCopyWithin(0,  2,  5, -5, [ 0, 1, 2, 3, 2, 3, 4, 7, 8 ]);
    checkCopyWithin(0, -7, -4, -5, [ 0, 1, 2, 3, 2, 3, 4, 7, 8 ]);

    
    checkCopyWithin(2, 0, 3, 4, [ 2, 3, 4, 5, 2, 3, 4 ]);

    
    checkCopyWithin(0,  5000,  6000, 0, [ 0, 1, 2, 3, 4, 5, 6, 7, 8 ]);
    checkCopyWithin(0, -5000, -6000, 0, [ 0, 1, 2, 3, 4, 5, 6, 7, 8 ]);
    checkCopyWithin(0, -5000,  6000, 0, [ 0, 1, 2, 3, 4, 5, 6, 7, 8 ]);
    checkCopyWithin(0,  5000,  6000, 1, [ 0, 1, 2, 3, 4, 5, 6, 7, 8 ]);
    checkCopyWithin(0, -5000, -6000, 1, [ 0, 1, 2, 3, 4, 5, 6, 7, 8 ]);
    checkCopyWithin(0,  5000,  6000, 0, [ 0, 1, 2, 3, 4, 5, 6, 7, 8 ]);
    checkCopyWithin(2, -5000, -6000, 0, [ 2, 3, 4, 5, 6, 7, 8 ]);
    checkCopyWithin(2, -5000,  6000, 0, [ 2, 3, 4, 5, 6, 7, 8 ]);
    checkCopyWithin(2,  5000,  6000, 1, [ 2, 3, 4, 5, 6, 7, 8 ]);
    checkCopyWithin(2, -5000, -6000, 1, [ 2, 3, 4, 5, 6, 7, 8 ]);

    checkCopyWithin(2, -5000,    3, 1,     [ 2, 2, 3, 4, 6, 7, 8 ]);
    checkCopyWithin(2,     1, 6000, 0,     [ 3, 4, 5, 6, 7, 8, 8 ]);
    checkCopyWithin(2,     1, 6000, -4000, [ 3, 4, 5, 6, 7, 8, 8 ]);

    testBufferManagement();

    print ("done");

    reportCompare(0, TestFailCount, "typed array tests");

    exitFunc ('test');
}
