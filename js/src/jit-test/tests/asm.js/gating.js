



































function module_b(stdlib, foreign, heap) {
    "use asm";

    
    var i32a = new stdlib.SharedInt32Array(heap);
    var ld = stdlib.Atomics.load;

    function do_load() {
	var v = 0;
	v = ld(i32a, 0)|0;
	return v|0;
    }

    return { load: do_load };
}

if (this.SharedArrayBuffer)
    assertEq(isAsmJSModule(module_b), true);

function module_d(stdlib, foreign, heap) {
    "use asm";

    var i32a = new stdlib.Int32Array(heap);
    var ld = stdlib.Atomics.load;

    function do_load() {
	var v = 0;
	
	
	v = ld(i32a, 0)|0;
	return v|0;
    }

    return { load: do_load };
}


