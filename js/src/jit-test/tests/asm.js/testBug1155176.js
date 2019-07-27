if (!this.SharedArrayBuffer || !isAsmJSCompilationAvailable())
    quit(0);





function m(stdlib, ffi, heap) {
    "use asm";

    var view = new stdlib.SharedUint32Array(heap);
    var cas = stdlib.Atomics.compareExchange;
    var hi = ffi.hi;

    function run() {
	hi(+(cas(view, 37, 0, 0)>>>0));
    }

    return run;
}

assertEq(isAsmJSModule(m), true);

function nonm(stdlib, ffi, heap) {

    var view = new stdlib.SharedUint32Array(heap);
    var cas = stdlib.Atomics.compareExchange;
    var hi = ffi.hi;

    function run() {
	hi(+cas(view, 37, 0, 0));
    }

    return run;
}

var sab = new SharedArrayBuffer(65536);
var ua = new SharedUint32Array(sab);
var results = [];
var mrun = m(this, {hi: function (x) { results.push(x) }}, sab);
var nonmrun = nonm(this, {hi: function (x) { results.push(x) }}, sab);

ua[37] = 0x80000001;

mrun();
nonmrun();

assertEq(results[0], ua[37]);
assertEq(results[0], results[1]);
