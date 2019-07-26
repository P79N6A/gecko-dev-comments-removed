



function test() {
    
    
    
    for (var size of [0, 8, 16, 200, 1000, 4096, -8, -200, -8192, -65536]) {
        var buffer_ctor = (size < 0) ? AsmJSArrayBuffer : ArrayBuffer;
        size = Math.abs(size);

        var old = buffer_ctor(size);
        var copy = deserialize(serialize(old, [old]));
        assertEq(old.byteLength, 0);
        assertEq(copy.byteLength, size);

        var constructors = [ Int8Array,
                             Uint8Array,
                             Int16Array,
                             Uint16Array,
                             Int32Array,
                             Uint32Array,
                             Float32Array,
                             Float64Array,
                             Uint8ClampedArray ];

        for (var ctor of constructors) {
            var buf = buffer_ctor(size);
            var old_arr = ctor(buf);
            assertEq(buf.byteLength, size);
            assertEq(buf, old_arr.buffer);
            assertEq(old_arr.length, size / old_arr.BYTES_PER_ELEMENT);

            var copy_arr = deserialize(serialize(old_arr, [ buf ]));
            assertEq(buf.byteLength, 0, "donor array buffer should be neutered");
            assertEq(old_arr.length, 0, "donor typed array should be neutered");
            assertEq(copy_arr.buffer.byteLength == size, true);
            assertEq(copy_arr.length, size / old_arr.BYTES_PER_ELEMENT);

            buf = null;
            old_arr = null;
            gc(); 
        }

        for (var ctor of constructors) {
            var buf = buffer_ctor(size);
            var old_arr = ctor(buf);
            var dv = DataView(buf); 
            var copy_arr = deserialize(serialize(old_arr, [ buf ]));
            assertEq(buf.byteLength, 0, "donor array buffer should be neutered");
            assertEq(old_arr.length, 0, "donor typed array should be neutered");
            assertEq(dv.byteLength, 0, "all views of donor array buffer should be neutered");

            buf = null;
            old_arr = null;
            gc(); 
        }

        
        if (size >= 4) {
            old = buffer_ctor(size);
            var view = Int32Array(old);
            view[0] = 1;
            var mutator = { get foo() { view[0] = 2; } };
            var copy = deserialize(serialize([ old, mutator ], [old]));
            var viewCopy = Int32Array(copy[0]);
            assertEq(view.length, 0); 
            assertEq(viewCopy[0], 2);
        }

        
        if (size >= 4) {
            old = buffer_ctor(size);
            var mutator = {
                get foo() {
                    deserialize(serialize(old, [old]));
                }
            };
            
            
        }
    }
}

test();
reportCompare(0, 0, 'ok');
