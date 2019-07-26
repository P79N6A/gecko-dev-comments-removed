



"use strict";

if (typeof Components != "undefined") {
  throw new Error("This file is meant to be loaded in a worker");
}
if (!module || !exports) {
  throw new Error("Please load this module with require()");
}

let SharedAll = require("resource://gre/modules/osfile/osfile_shared_allthreads.jsm");
let libxul = new SharedAll.Library("libxul", SharedAll.Constants.Path.libxul);
let Type = SharedAll.Type;

let Primitives = {};

libxul.declareLazyFFI(Primitives, "compress",
  "workerlz4_compress",
  null,
   Type.size_t,
   Type.void_t.in_ptr,
   Type.size_t,
   Type.void_t.out_ptr
);

libxul.declareLazyFFI(Primitives, "decompress",
  "workerlz4_decompress",
  null,
   Type.int,
   Type.void_t.in_ptr,
   Type.size_t,
   Type.void_t.out_ptr,
   Type.size_t,
   Type.size_t.out_ptr
);

libxul.declareLazyFFI(Primitives, "maxCompressedSize",
  "workerlz4_maxCompressedSize",
  null,
   Type.size_t,
   Type.size_t
);

module.exports = {
  get compress() {
    return Primitives.compress;
  },
  get decompress() {
    return Primitives.decompress;
  },
  get maxCompressedSize() {
    return Primitives.maxCompressedSize;
  }
};
