













"use strict";

importScripts("resource://gre/modules/workers/require.js");

let SharedAll = require(
  "resource://gre/modules/osfile/osfile_shared_allthreads.jsm");


let path;
if (SharedAll.Constants.Sys.Name === "Android") {
  path = ctypes.libraryName("sqlite3");
} else if (SharedAll.Constants.Win) {
  path = ctypes.libraryName("nss3");
} else {
  path = SharedAll.Constants.Path.libxul;
}

let lib;
try {
  lib = ctypes.open(path);
} catch (ex) {
  throw new Error("Could not open system library: " + ex.message);
}

let declareLazyFFI = SharedAll.declareLazyFFI;

let Type = Object.create(SharedAll.Type);





Type.sqlite3_ptr = Type.voidptr_t.withName("sqlite3_ptr");






Type.sqlite3_stmt_ptr = Type.voidptr_t.withName("sqlite3_stmt_ptr");





Type.sqlite3_destructor_ptr = Type.voidptr_t.withName(
  "sqlite3_destructor_ptr");




Type.double = new SharedAll.Type("double", ctypes.double);




Type.sqlite3_int64 = Type.int64_t.withName("sqlite3_int64");




let Constants = {};







Constants.SQLITE_STATIC = Type.sqlite3_destructor_ptr.implementation(0);







Constants.SQLITE_TRANSIENT = Type.sqlite3_destructor_ptr.implementation(-1);





Constants.SQLITE_OK = 0;





Constants.SQLITE_ROW = 100;





Constants.SQLITE_DONE = 101;

let Sqlite3 = {
  Constants: Constants,
  Type: Type
};

declareLazyFFI(Sqlite3, "open", lib, "sqlite3_open", null,
                   Type.int,
                     Type.char.in_ptr,
                Type.sqlite3_ptr.out_ptr);

declareLazyFFI(Sqlite3, "open_v2", lib, "sqlite3_open_v2", null,
                   Type.int,
                     Type.char.in_ptr,
                Type.sqlite3_ptr.out_ptr,
                    Type.int,
                      Type.char.in_ptr);

declareLazyFFI(Sqlite3, "close", lib, "sqlite3_close", null,
                   Type.int,
                Type.sqlite3_ptr);

declareLazyFFI(Sqlite3, "prepare_v2", lib, "sqlite3_prepare_v2", null,
                   Type.int,
                Type.sqlite3_ptr,
                     Type.char.in_ptr,
                    Type.int,
                Type.sqlite3_stmt_ptr.out_ptr,
                   Type.cstring.out_ptr);

declareLazyFFI(Sqlite3, "step", lib, "sqlite3_step", null,
                   Type.int,
                Type.sqlite3_stmt_ptr);

declareLazyFFI(Sqlite3, "finalize", lib, "sqlite3_finalize", null,
                   Type.int,
                Type.sqlite3_stmt_ptr);

declareLazyFFI(Sqlite3, "reset", lib, "sqlite3_reset", null,
                   Type.int,
                Type.sqlite3_stmt_ptr);

declareLazyFFI(Sqlite3, "column_int", lib, "sqlite3_column_int", null,
                   Type.int,
                Type.sqlite3_stmt_ptr,
                      Type.int);

declareLazyFFI(Sqlite3, "column_blob", lib, "sqlite3_column_blob", null,
                   Type.voidptr_t,
                Type.sqlite3_stmt_ptr,
                      Type.int);

declareLazyFFI(Sqlite3, "column_bytes", lib, "sqlite3_column_bytes", null,
                   Type.int,
                Type.sqlite3_stmt_ptr,
                      Type.int);

declareLazyFFI(Sqlite3, "column_bytes16", lib, "sqlite3_column_bytes16",
                             null,
                   Type.int,
                Type.sqlite3_stmt_ptr,
                      Type.int);

declareLazyFFI(Sqlite3, "column_double", lib, "sqlite3_column_double", null,
                   Type.double,
                Type.sqlite3_stmt_ptr,
                      Type.int);

declareLazyFFI(Sqlite3, "column_int64", lib, "sqlite3_column_int64", null,
                   Type.sqlite3_int64,
                Type.sqlite3_stmt_ptr,
                      Type.int);

declareLazyFFI(Sqlite3, "column_text", lib, "sqlite3_column_text", null,
                   Type.cstring,
                Type.sqlite3_stmt_ptr,
                      Type.int);

declareLazyFFI(Sqlite3, "column_text16", lib, "sqlite3_column_text16", null,
                   Type.wstring,
                Type.sqlite3_stmt_ptr,
                      Type.int);

declareLazyFFI(Sqlite3, "bind_int", lib, "sqlite3_bind_int", null,
                   Type.int,
                Type.sqlite3_stmt_ptr,
                    Type.int,
                    Type.int);

declareLazyFFI(Sqlite3, "bind_int64", lib, "sqlite3_bind_int64", null,
                   Type.int,
                Type.sqlite3_stmt_ptr,
                    Type.int,
                    Type.sqlite3_int64);

declareLazyFFI(Sqlite3, "bind_double", lib, "sqlite3_bind_double", null,
                   Type.int,
                Type.sqlite3_stmt_ptr,
                    Type.int,
                    Type.double);

declareLazyFFI(Sqlite3, "bind_null", lib, "sqlite3_bind_null", null,
                   Type.int,
                Type.sqlite3_stmt_ptr,
                    Type.int);

declareLazyFFI(Sqlite3, "bind_zeroblob", lib, "sqlite3_bind_zeroblob", null,
                   Type.int,
                Type.sqlite3_stmt_ptr,
                    Type.int,
                   Type.int);

declareLazyFFI(Sqlite3, "bind_text", lib, "sqlite3_bind_text", null,
                    Type.int,
                 Type.sqlite3_stmt_ptr,
                     Type.int,
                     Type.cstring,
                    Type.int,
                Type.sqlite3_destructor_ptr);

declareLazyFFI(Sqlite3, "bind_text16", lib, "sqlite3_bind_text16", null,
                    Type.int,
                 Type.sqlite3_stmt_ptr,
                     Type.int,
                     Type.wstring,
                    Type.int,
                Type.sqlite3_destructor_ptr);

declareLazyFFI(Sqlite3, "bind_blob", lib, "sqlite3_bind_blob", null,
                    Type.int,
                 Type.sqlite3_stmt_ptr,
                     Type.int,
                     Type.voidptr_t,
                    Type.int,
                Type.sqlite3_destructor_ptr);

module.exports = {
  get Constants() {
    return Sqlite3.Constants;
  },
  get Type() {
    return Sqlite3.Type;
  },
  get open() {
    return Sqlite3.open;
  },
  get open_v2() {
    return Sqlite3.open_v2;
  },
  get close() {
    return Sqlite3.close;
  },
  get prepare_v2() {
    return Sqlite3.prepare_v2;
  },
  get step() {
    return Sqlite3.step;
  },
  get finalize() {
    return Sqlite3.finalize;
  },
  get reset() {
    return Sqlite3.reset;
  },
  get column_int() {
    return Sqlite3.column_int;
  },
  get column_blob() {
    return Sqlite3.column_blob;
  },
  get column_bytes() {
    return Sqlite3.column_bytes;
  },
  get column_bytes16() {
    return Sqlite3.column_bytes16;
  },
  get column_double() {
    return Sqlite3.column_double;
  },
  get column_int64() {
    return Sqlite3.column_int64;
  },
  get column_text() {
    return Sqlite3.column_text;
  },
  get column_text16() {
    return Sqlite3.column_text16;
  },
  get bind_int() {
    return Sqlite3.bind_int;
  },
  get bind_int64() {
    return Sqlite3.bind_int64;
  },
  get bind_double() {
    return Sqlite3.bind_double;
  },
  get bind_null() {
    return Sqlite3.bind_null;
  },
  get bind_zeroblob() {
    return Sqlite3.bind_zeroblob;
  },
  get bind_text() {
    return Sqlite3.bind_text;
  },
  get bind_text16() {
    return Sqlite3.bind_text16;
  },
  get bind_blob() {
    return Sqlite3.bind_blob;
  }
};
