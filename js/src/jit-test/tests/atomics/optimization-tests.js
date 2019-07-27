












if (!(this.Atomics && this.SharedArrayBuffer && this.SharedInt8Array))
    quit(0);

var sum = 0;

function f(ia, k) {
    
    
    
    
    Atomics.add(ia, 0, k);

    
    
    Atomics.add(ia, 0, 1);
}

function f2(ia, k) {
    
    
    Atomics.sub(ia, 2, k);

    
    
    Atomics.sub(ia, 2, 1);
}

function g(ia, k) {
    
    
    sum += Atomics.add(ia, 1, k);

    
    
    
    sum += Atomics.add(ia, 1, 1);
}

function g2(ia, k) {
    
    
    
    sum += Atomics.sub(ia, 3, k);

    
    
    
    sum += Atomics.sub(ia, 3, 1);
}

function mod(stdlib, ffi, heap) {
    "use asm";

    var i8a = new stdlib.SharedInt8Array(heap);
    var add = stdlib.Atomics.add;
    var sum = 0;

    function f3(k) {
	k = k|0;
	add(i8a, 4, 1);
	add(i8a, 4, k);
    }

    function g3(k) {
	k = k|0;
	sum = sum + add(i8a, 5, k)|0;
	sum = sum + add(i8a, 5, 1)|0;
    }

    return {f3:f3, g3:g3};
}

var i8a = new SharedInt8Array(65536);
var { f3, g3 } = mod(this, {}, i8a.buffer);
for ( var i=0 ; i < 10000 ; i++ ) {
    f(i8a, i % 10);
    g(i8a, i % 10);
    f2(i8a, i % 10);
    g2(i8a, i % 10);
    f3(i % 10);
    g3(i % 10);
}

assertEq(i8a[0], ((10000 + 10000*4.5) << 24) >> 24);
assertEq(i8a[1], ((10000 + 10000*4.5) << 24) >> 24);
assertEq(i8a[2], ((-10000 + -10000*4.5) << 24) >> 24);
assertEq(i8a[3], ((-10000 + -10000*4.5) << 24) >> 24);
assertEq(i8a[4], ((10000 + 10000*4.5) << 24) >> 24);
assertEq(i8a[5], ((10000 + 10000*4.5) << 24) >> 24);
