
if (!this.Atomics)
    quit();

function m(stdlib, ffi, heap)
{
    "use asm";

    var HEAP32 = new stdlib.SharedInt32Array(heap);
    var add = stdlib.Atomics.add;
    var load = stdlib.Atomics.load;
    var _emscripten_asm_const_int=ffi._emscripten_asm_const_int;

    
    
    

    function add_sharedEv(i1) {
	i1 = i1 | 0;
	var i2 = 0;
	var xx = 0;
	i2 = i1 + 4 | 0;
	i1 = load(HEAP32, i2 >> 2) | 0;
	_emscripten_asm_const_int(7, i2 | 0, i1 | 0) | 0;
	add(HEAP32, i2 >> 2, 1) | 0;
	_emscripten_asm_const_int(8, i2 | 0, load(HEAP32, i2 >> 2) | 0, i1 + 1 | 0) | 0;
	return xx|0;
    }

    return {add_sharedEv:add_sharedEv};
}

if (isAsmJSCompilationAvailable())
    assertEq(isAsmJSModule(m), true);

var x;

var sab = new SharedArrayBuffer(65536);
var ffi =
    { _emscripten_asm_const_int:
      function (...rest) {
	  
	  if (rest[0] == 8)
	      x = rest[2];
      }
    };
var {add_sharedEv} = m(this, ffi, sab);
add_sharedEv(13812);

assertEq(x, 1);
