






var DEBUG = false;		

function dprint(...xs) {
    if (!DEBUG)
	return;
    var s = "";
    for ( var x in xs )
	s += String(xs[x]);
    print(s);
}

function testMethod(a, ...indices) {
    dprint("Method: " + a.constructor.name);
    var poison;
    switch (a.BYTES_PER_ELEMENT) {
    case 1: poison = 0x5A; break;
    case 2: poison = 0x5A5A; break;
    case 4: poison = 0x5A5A5A5A; break;
    }
    for ( var i=0 ; i < indices.length ; i++ ) {
	var x = indices[i];
	if (x > 0)
	    a[x-1] = poison;
	if (x < a.length-1)
	    a[x+1] = poison;

	
	assertEq(Atomics.compareExchange(a, x, 0, 37), 0);
	
	assertEq(Atomics.compareExchange(a, x, 37, 5), 37);
	
	assertEq(Atomics.compareExchange(a, x, 7, 8), 5); 
	
	assertEq(Atomics.compareExchange(a, x, 5, 9), 5);
	
	assertEq(Atomics.compareExchange(a, x, 5, 0), 9); 

	
	assertEq(Atomics.load(a, x), 9);
	
	assertEq(Atomics.store(a, x, 14), 14); 
	
	assertEq(Atomics.load(a, x), 14);
	
	Atomics.store(a, x, 0);
	

	Atomics.fence();

	
	assertEq(Atomics.add(a, x, 3), 0);
	
	assertEq(Atomics.sub(a, x, 2), 3);
	
	assertEq(Atomics.or(a, x, 6), 1);
	
	assertEq(Atomics.and(a, x, 14), 7);
	
	assertEq(Atomics.xor(a, x, 5), 6);
	
	assertEq(Atomics.load(a, x), 3);
	
	Atomics.store(a, x, 0);
	

	
	if (x > 0) {
	    assertEq(a[x-1], poison);
	    a[x-1] = 0;
	}
	if (x < a.length-1) {
	    assertEq(a[x+1], poison);
	    a[x+1] = 0;
	}
    }
}

function testFunction(a, ...indices) {
    dprint("Function: " + a.constructor.name);
    var poison;
    switch (a.BYTES_PER_ELEMENT) {
    case 1: poison = 0x5A; break;
    case 2: poison = 0x5A5A; break;
    case 4: poison = 0x5A5A5A5A; break;
    }
    for ( var i=0 ; i < indices.length ; i++ ) {
	var x = indices[i];
	if (x > 0)
	    a[x-1] = poison;
	if (x < a.length-1)
	    a[x+1] = poison;

	
	assertEq(gAtomics_compareExchange(a, x, 0, 37), 0);
	
	assertEq(gAtomics_compareExchange(a, x, 37, 5), 37);
	
	assertEq(gAtomics_compareExchange(a, x, 7, 8), 5); 
	
	assertEq(gAtomics_compareExchange(a, x, 5, 9), 5);
	
	assertEq(gAtomics_compareExchange(a, x, 5, 0), 9); 

	
	assertEq(gAtomics_load(a, x), 9);
	
	assertEq(gAtomics_store(a, x, 14), 14); 
	
	assertEq(gAtomics_load(a, x), 14);
	
	gAtomics_store(a, x, 0);
	

	gAtomics_fence();

	
	assertEq(gAtomics_add(a, x, 3), 0);
	
	assertEq(gAtomics_sub(a, x, 2), 3);
	
	assertEq(gAtomics_or(a, x, 6), 1);
	
	assertEq(gAtomics_and(a, x, 14), 7);
	
	assertEq(gAtomics_xor(a, x, 5), 6);
	
	assertEq(gAtomics_load(a, x), 3);
	
	gAtomics_store(a, x, 0);
	

	
	if (x > 0) {
	    assertEq(a[x-1], poison);
	    a[x-1] = 0;
	}
	if (x < a.length-1) {
	    assertEq(a[x+1], poison);
	    a[x+1] = 0;
	}
    }
}

function testTypeCAS(a) {
    dprint("Type: " + a.constructor.name);

    var thrown = false;
    try {
	Atomics.compareExchange([0], 0, 0, 1);
    }
    catch (e) {
	thrown = true;
	assertEq(e instanceof TypeError, true);
    }
    assertEq(thrown, true);

    
    Atomics.compareExchange(a, 0, 0.7, 1.8);
    Atomics.compareExchange(a, 0, "0", 1);
    Atomics.compareExchange(a, 0, 0, "1");
    Atomics.compareExchange(a, 0, 0);
}

function testTypeBinop(a, op) {
    dprint("Type: " + a.constructor.name);

    var thrown = false;
    try {
	op([0], 0, 1);
    }
    catch (e) {
	thrown = true;
	assertEq(e instanceof TypeError, true);
    }
    assertEq(thrown, true);

    
    op(a, 0, 0.7);
    op(a, 0, "0");
    op(a, 0);
}

function testRangeCAS(a) {
    dprint("Range: " + a.constructor.name);

    assertEq(Atomics.compareExchange(a, -1, 0, 1), undefined); 
    assertEq(a[0], 0);
    a[0] = 0;

    assertEq(Atomics.compareExchange(a, "hi", 0, 1), undefined); 
    assertEq(a[0], 0);
    a[0] = 0;

    assertEq(Atomics.compareExchange(a, a.length + 5, 0, 1), undefined); 
    assertEq(a[0], 0);
}




function testInt8Extremes(a) {
    dprint("Int8 extremes");

    a[10] = 0;
    a[11] = 0;

    Atomics.store(a, 10, 255);
    assertEq(a[10], -1);
    assertEq(Atomics.load(a, 10), -1);

    Atomics.add(a, 10, 255); 
    assertEq(a[10], -2);
    assertEq(Atomics.load(a, 10), -2);

    Atomics.add(a, 10, -1);
    assertEq(a[10], -3);
    assertEq(Atomics.load(a, 10), -3);

    Atomics.sub(a, 10, 255);	
    assertEq(a[10], -2);
    assertEq(Atomics.load(a, 10), -2);

    Atomics.sub(a, 10, 256);	
    assertEq(a[10], -2);
    assertEq(Atomics.load(a, 10), -2);

    Atomics.and(a, 10, -1);	
    assertEq(a[10], -2);
    assertEq(Atomics.load(a, 10), -2);

    Atomics.and(a, 10, 256);	
    assertEq(a[10], 0);
    assertEq(Atomics.load(a, 10), 0);
    
    assertEq(a[11], 0);
}

function testUint8Extremes(a) {
    dprint("Uint8 extremes");

    a[10] = 0;
    a[11] = 0;

    Atomics.store(a, 10, 255);
    assertEq(a[10], 255);
    assertEq(Atomics.load(a, 10), 255);

    Atomics.add(a, 10, 255);
    assertEq(a[10], 254);
    assertEq(Atomics.load(a, 10), 254);

    Atomics.add(a, 10, -1);
    assertEq(a[10], 253);
    assertEq(Atomics.load(a, 10), 253);

    Atomics.sub(a, 10, 255);
    assertEq(a[10], 254);
    assertEq(Atomics.load(a, 10), 254);

    Atomics.and(a, 10, -1);	
    assertEq(a[10], 254);
    assertEq(Atomics.load(a, 10), 254);

    Atomics.and(a, 10, 256);	
    assertEq(a[10], 0);
    assertEq(Atomics.load(a, 10), 0);
    
    assertEq(a[11], 0);
}

function testInt16Extremes(a) {
    dprint("Int16 extremes");

    a[10] = 0;
    a[11] = 0;

    Atomics.store(a, 10, 65535);
    assertEq(a[10], -1);
    assertEq(Atomics.load(a, 10), -1);

    Atomics.add(a, 10, 65535); 
    assertEq(a[10], -2);
    assertEq(Atomics.load(a, 10), -2);

    Atomics.add(a, 10, -1);
    assertEq(a[10], -3);
    assertEq(Atomics.load(a, 10), -3);

    Atomics.sub(a, 10, 65535);	
    assertEq(a[10], -2);
    assertEq(Atomics.load(a, 10), -2);

    Atomics.sub(a, 10, 65536);	
    assertEq(a[10], -2);
    assertEq(Atomics.load(a, 10), -2);

    Atomics.and(a, 10, -1);	
    assertEq(a[10], -2);
    assertEq(Atomics.load(a, 10), -2);

    Atomics.and(a, 10, 65536);	
    assertEq(a[10], 0);
    assertEq(Atomics.load(a, 10), 0);

    assertEq(a[11], 0);
}

function isLittleEndian() {
    var xxx = new ArrayBuffer(2);
    var xxa = new Int16Array(xxx);
    var xxb = new Int8Array(xxx);
    xxa[0] = 37;
    var is_little = xxb[0] == 37;
    return is_little;
}

function runTests() {
    var is_little = isLittleEndian();

    
    var sab = new SharedArrayBuffer(4096);

    
    var t1 = new SharedInt8Array(sab);
    var t2 = new SharedUint16Array(sab);

    assertEq(t1[0], 0);
    assertEq(t2[0], 0);
    t1[0] = 37;
    if (is_little)
	assertEq(t2[0], 37);
    else
	assertEq(t2[0], 37 << 16);
    t1[0] = 0;

    
    testMethod(new SharedInt8Array(sab), 0, 42, 4095);
    testMethod(new SharedUint8Array(sab), 0, 42, 4095);
    testMethod(new SharedUint8ClampedArray(sab), 0, 42, 4095);
    testMethod(new SharedInt16Array(sab), 0, 42, 2047);
    testMethod(new SharedUint16Array(sab), 0, 42, 2047);
    testMethod(new SharedInt32Array(sab), 0, 42, 1023);
    testMethod(new SharedUint32Array(sab), 0, 42, 1023);

    
    gAtomics_compareExchange = Atomics.compareExchange;
    gAtomics_load = Atomics.load;
    gAtomics_store = Atomics.store;
    gAtomics_fence = Atomics.fence;
    gAtomics_add = Atomics.add;
    gAtomics_sub = Atomics.sub;
    gAtomics_and = Atomics.and;
    gAtomics_or = Atomics.or;
    gAtomics_xor = Atomics.xor;

    testFunction(new SharedInt8Array(sab), 0, 42, 4095);
    testFunction(new SharedUint8Array(sab), 0, 42, 4095);
    testFunction(new SharedUint8ClampedArray(sab), 0, 42, 4095);
    testFunction(new SharedInt16Array(sab), 0, 42, 2047);
    testFunction(new SharedUint16Array(sab), 0, 42, 2047);
    testFunction(new SharedInt32Array(sab), 0, 42, 1023);
    testFunction(new SharedUint32Array(sab), 0, 42, 1023);

    
    var v8 = new SharedInt8Array(sab);
    var v32 = new SharedInt32Array(sab);

    testTypeCAS(v8);
    testTypeCAS(v32);

    testTypeBinop(v8, Atomics.add);
    testTypeBinop(v8, Atomics.sub);
    testTypeBinop(v8, Atomics.and);
    testTypeBinop(v8, Atomics.or);
    testTypeBinop(v8, Atomics.xor);

    testTypeBinop(v32, Atomics.add);
    testTypeBinop(v32, Atomics.sub);
    testTypeBinop(v32, Atomics.and);
    testTypeBinop(v32, Atomics.or);
    testTypeBinop(v32, Atomics.xor);

    
    testRangeCAS(v8);
    testRangeCAS(v32);

    
    testInt8Extremes(new SharedInt8Array(sab));
    testUint8Extremes(new SharedUint8Array(sab));
    testInt16Extremes(new SharedInt16Array(sab));
}

if (this.Atomics && this.SharedArrayBuffer && this.SharedInt32Array)
    runTests();
