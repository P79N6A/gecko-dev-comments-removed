







g = (function(stdlib, n, heap) {
    "use asm";
    var Float32ArrayView = new stdlib.Float32Array(heap);
    function f() {
        return +Float32ArrayView[0]
    }
    return f
})(this, {}, new SharedArrayBuffer(4096));
assertEq(g(), NaN);



try {
    g = (function(stdlib, n, heap) {
	"use asm";
	var Float32ArrayView = new stdlib.SharedFloat32Array(heap);
	function f() {
            return +Float32ArrayView[0]
	}
	return f
    })(this, {}, new ArrayBuffer(4096));
    
    assertEq(g(), NaN);
}
catch (e) {
    
    
    
    
}
