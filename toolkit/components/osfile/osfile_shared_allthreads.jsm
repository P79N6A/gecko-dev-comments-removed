



{
  if (typeof Components != "undefined") {
    this.EXPORTED_SYMBOLS = ["OS"];
  }
  (function(exports) {
     "use strict";
     




     if (!exports.OS) {
       exports.OS = {};
     }
     if (!exports.OS.Shared) {
       exports.OS.Shared = {};
     }
     if (exports.OS.Shared.Type) {
       return; 
     }

     
     
     if (typeof Components != "undefined") {
       const Cu = Components.utils;
       Cu.import("resource://gre/modules/ctypes.jsm");
       Components.classes["@mozilla.org/net/osfileconstantsservice;1"].
         getService(Components.interfaces.nsIOSFileConstantsService).init();

       if (typeof exports.OS.Shared.DEBUG !== "undefined") {
         return; 
       }

       Cu.import("resource://gre/modules/Services.jsm");

       const PREF_OSFILE_LOG = "toolkit.osfile.log";

       






       let readDebugPref = function readDebugPref(oldPref) {
         let pref;
         try {
           pref = Services.prefs.getBoolPref(PREF_OSFILE_LOG);
         } catch (x) {
           
           pref = oldPref;
         }
         
         return pref || false;
       };

       


       exports.OS.Shared.DEBUG = readDebugPref(exports.OS.Shared.DEBUG);

       



       Services.prefs.addObserver(PREF_OSFILE_LOG,
         function prefObserver(aSubject, aTopic, aData) {
           exports.OS.Shared.DEBUG = readDebugPref(exports.OS.Shared.DEBUG);
         }, false);
     }

     
     let defineLazyGetter = function defineLazyGetter(object, name, getter) {
       Object.defineProperty(object, name, {
         configurable: true,
         get: function lazy() {
           delete this[name];
           let value = getter.call(this);
           Object.defineProperty(object, name, {
             value: value
           });
           return value;
         }
       });
     };
     exports.OS.Shared.defineLazyGetter = defineLazyGetter;

     let LOG;
     if (typeof console != "undefined" && console.log) {
       LOG = console.log.bind(console, "OS");
     } else {
       LOG = function() {
         let text = "OS";
         for (let i = 0; i < arguments.length; ++i) {
           text += (" " + arguments[i]);
         }
         dump(text + "\n");
       };
     }
     exports.OS.Shared.LOG = LOG;

     










     function OSError(operation) {
       Error.call(this);
       this.operation = operation;
     }
     exports.OS.Shared.Error = OSError;

     












     function Type(name, implementation) {
       if (!(typeof name == "string")) {
         throw new TypeError("Type expects as first argument a name, got: "
                             + name);
       }
       if (!(implementation instanceof ctypes.CType)) {
         throw new TypeError("Type expects as second argument a ctypes.CType"+
                             ", got: " + implementation);
       }
       Object.defineProperty(this, "name", { value: name });
       Object.defineProperty(this, "implementation", { value: implementation });
     }
     Type.prototype = {
       






       toMsg: function default_toMsg(value) {
         return value;
       },
       





       fromMsg: function default_fromMsg(msg) {
         return msg;
       },
       





       importFromC: function default_importFromC(value) {
         return value;
       },

       


       get in_ptr() {
         delete this.in_ptr;
         let ptr_t = new PtrType(
           "[in] " + this.name + "*",
           this.implementation.ptr,
           this);
         Object.defineProperty(this, "in_ptr",
           {
             get: function() {
               return ptr_t;
             }
           });
         return ptr_t;
       },

       


       get out_ptr() {
         delete this.out_ptr;
         let ptr_t = new PtrType(
           "[out] " + this.name + "*",
           this.implementation.ptr,
           this);
         Object.defineProperty(this, "out_ptr",
           {
             get: function() {
               return ptr_t;
             }
           });
         return ptr_t;
       },

       






       get inout_ptr() {
         delete this.inout_ptr;
         let ptr_t = new PtrType(
           "[inout] " + this.name + "*",
           this.implementation.ptr,
           this);
         Object.defineProperty(this, "inout_ptr",
           {
             get: function() {
               return ptr_t;
             }
           });
         return ptr_t;
       },

       


       releaseWith: function releaseWith(finalizer) {
         let parent = this;
         let type = this.withName("[auto " + this.name + ", " + finalizer + "] ");
         type.importFromC = function importFromC(value, operation) {
           return ctypes.CDataFinalizer(
             parent.importFromC(value, operation),
             finalizer);
         };
         return type;
       },

       


       withName: function withName(name) {
         return Object.create(this, {name: {value: name}});
       },

       




       cast: function cast(value) {
         return ctypes.cast(value, this.implementation);
       },

       





       get size() {
         return this.implementation.size;
       }
     };

     


     let isTypedArray = function isTypedArray(obj) {
       return typeof obj == "object"
         && "byteOffset" in obj;
     };
     exports.OS.Shared.isTypedArray = isTypedArray;

     






     function PtrType(name, implementation, targetType) {
       Type.call(this, name, implementation);
       if (targetType == null || !targetType instanceof Type) {
         throw new TypeError("targetType must be an instance of Type");
       }
       


       Object.defineProperty(this, "targetType", {
         value: targetType
       });
     }
     PtrType.prototype = Object.create(Type.prototype);

     









     PtrType.prototype.toMsg = function ptr_toMsg(value) {
       if (value == null) {
         return null;
       }
       if (typeof value == "string") {
         return { string: value };
       }
       let normalized;
       if (isTypedArray(value)) { 
         normalized = Types.uint8_t.in_ptr.implementation(value.buffer);
         if (value.byteOffset != 0) {
           normalized = exports.OS.Shared.offsetBy(normalized, value.byteOffset);
         }
       } else if ("addressOfElement" in value) { 
         normalized = value.addressOfElement(0);
       } else if ("isNull" in value) { 
         normalized = value;
       } else {
         throw new TypeError("Value " + value +
           " cannot be converted to a pointer");
       }
       let cast = Types.uintptr_t.cast(normalized);
       return {ptr: cast.value.toString()};
     };

     


     PtrType.prototype.fromMsg = function ptr_fromMsg(msg) {
       if (msg == null) {
         return null;
       }
       if ("string" in msg) {
         return msg.string;
       }
       if ("ptr" in msg) {
         let address = ctypes.uintptr_t(msg.ptr);
         return this.cast(address);
       }
       throw new TypeError("Message " + msg.toSource() +
         " does not represent a pointer");
     };

     exports.OS.Shared.Type = Type;
     let Types = Type;

     





     let projectLargeInt = function projectLargeInt(x) {
       return parseInt(x.toString(), 10);
     };
     let projectLargeUInt = function projectLargeUInt(x) {
       return parseInt(x.toString(), 10);
     };
     let projectValue = function projectValue(x) {
       if (!(x instanceof ctypes.CData)) {
         return x;
       }
       if (!("value" in x)) { 
         throw new TypeError("Number " + x.toSource() + " has no field |value|");
       }
       return x.value;
     };

     function projector(type, signed) {
       if (exports.OS.Shared.DEBUG) {
         LOG("Determining best projection for", type,
             "(size: ", type.size, ")", signed?"signed":"unsigned");
       }
       if (type instanceof Type) {
         type = type.implementation;
       }
       if (!type.size) {
         throw new TypeError("Argument is not a proper C type");
       }
       
       if (type.size == 8           
           
           
           
           || type == ctypes.size_t 
           || type == ctypes.ssize_t
           || type == ctypes.intptr_t
           || type == ctypes.uintptr_t
           || type == ctypes.off_t){
          if (signed) {
	    if (exports.OS.Shared.DEBUG) {
             LOG("Projected as a large signed integer");
	    }
            return projectLargeInt;
          } else {
	    if (exports.OS.Shared.DEBUG) {
             LOG("Projected as a large unsigned integer");
	    }
            return projectLargeUInt;
          }
       }
       if (exports.OS.Shared.DEBUG) {
         LOG("Projected as a regular number");
       }
       return projectValue;
     };
     exports.OS.Shared.projectValue = projectValue;



     







     Types.uintn_t = function uintn_t(size) {
       switch (size) {
       case 1: return Types.uint8_t;
       case 2: return Types.uint16_t;
       case 4: return Types.uint32_t;
       case 8: return Types.uint64_t;
       default:
         throw new Error("Cannot represent unsigned integers of " + size + " bytes");
       }
     };

     







     Types.intn_t = function intn_t(size) {
       switch (size) {
       case 1: return Types.int8_t;
       case 2: return Types.int16_t;
       case 4: return Types.int32_t;
       case 8: return Types.int64_t;
       default:
         throw new Error("Cannot represent integers of " + size + " bytes");
       }
     };

     



     


     Types.void_t =
       new Type("void",
                ctypes.void_t);

     


     Types.voidptr_t =
       new PtrType("void*",
                   ctypes.voidptr_t,
                   Types.void_t);

     
     
     
     ["in_ptr", "out_ptr", "inout_ptr"].forEach(function(key) {
       Object.defineProperty(Types.void_t, key,
       {
         value: Types.voidptr_t
       });
     });

     









     function IntType(name, implementation, signed) {
       Type.call(this, name, implementation);
       this.importFromC = projector(implementation, signed);
       this.project = this.importFromC;
     };
     IntType.prototype = Object.create(Type.prototype);
     IntType.prototype.toMsg = function toMsg(value) {
       if (typeof value == "number") {
         return value;
       }
       return this.project(value);
     };

     


     Types.char =
       new Type("char",
                ctypes.char);

     


     Types.jschar =
       new Type("jschar",
                ctypes.jschar);

      


     Types.cstring = Types.char.in_ptr.withName("[in] C string");
     Types.wstring = Types.jschar.in_ptr.withName("[in] wide string");
     Types.out_cstring = Types.char.out_ptr.withName("[out] C string");
     Types.out_wstring = Types.jschar.out_ptr.withName("[out] wide string");

     


     Types.int8_t =
       new IntType("int8_t", ctypes.int8_t, true);

     Types.uint8_t =
       new IntType("uint8_t", ctypes.uint8_t, false);

     




     Types.int16_t =
       new IntType("int16_t", ctypes.int16_t, true);

     Types.uint16_t =
       new IntType("uint16_t", ctypes.uint16_t, false);

     




     Types.int32_t =
       new IntType("int32_t", ctypes.int32_t, true);

     Types.uint32_t =
       new IntType("uint32_t", ctypes.uint32_t, false);

     


     Types.int64_t =
       new IntType("int64_t", ctypes.int64_t, true);

     Types.uint64_t =
       new IntType("uint64_t", ctypes.uint64_t, false);

      




     Types.int = Types.intn_t(ctypes.int.size).
       withName("int");

     Types.unsigned_int = Types.intn_t(ctypes.unsigned_int.size).
       withName("unsigned int");

     




     Types.long =
       Types.intn_t(ctypes.long.size).withName("long");

     Types.unsigned_long =
       Types.intn_t(ctypes.unsigned_long.size).withName("unsigned long");

     




     Types.uintptr_t =
       Types.uintn_t(ctypes.uintptr_t.size).withName("uintptr_t");

     



     Types.bool = Types.int.withName("bool");
     Types.bool.importFromC = function projectBool(x) {
       return !!(x.value);
     };

     




     Types.uid_t =
       Types.int.withName("uid_t");

     




     Types.gid_t =
       Types.int.withName("gid_t");

     




     Types.off_t =
       new IntType("off_t", ctypes.off_t, true);

     




     Types.size_t =
       new IntType("size_t", ctypes.size_t, false);

     



     Types.ssize_t =
       new IntType("ssize_t", ctypes.ssize_t, true);

     


     Types.uencoder =
       new Type("uencoder", ctypes.StructType("uencoder"));
     Types.udecoder =
       new Type("udecoder", ctypes.StructType("udecoder"));

     






     function HollowStructure(name, size) {
       if (!name) {
         throw new TypeError("HollowStructure expects a name");
       }
       if (!size || size < 0) {
         throw new TypeError("HollowStructure expects a (positive) size");
       }

       
       
       this.offset_to_field_info = [];

       
       this.name = name;

       
       this.size = size;

       
       
       this._paddings = 0;
     }
     HollowStructure.prototype = {
       






       add_field_at: function add_field_at(offset, name, type) {
         if (offset == null) {
           throw new TypeError("add_field_at requires a non-null offset");
         }
         if (!name) {
           throw new TypeError("add_field_at requires a non-null name");
         }
         if (!type) {
           throw new TypeError("add_field_at requires a non-null type");
         }
         if (type instanceof Type) {
           type = type.implementation;
         }
         if (this.offset_to_field_info[offset]) {
           throw new Error("HollowStructure " + this.name +
                           " already has a field at offset " + offset);
         }
         if (offset + type.size > this.size) {
           throw new Error("HollowStructure " + this.name +
                           " cannot place a value of type " + type +
                           " at offset " + offset +
                           " without exceeding its size of " + this.size);
         }
         let field = {name: name, type:type};
         this.offset_to_field_info[offset] = field;
       },

       






       _makePaddingField: function makePaddingField(size) {
         let field = ({});
         field["padding_" + this._paddings] =
           ctypes.ArrayType(ctypes.uint8_t, size);
         this._paddings++;
         return field;
       },

       


       getType: function getType() {
         
         
         let struct = [];

         let i = 0;
         while (i < this.size) {
           let currentField = this.offset_to_field_info[i];
           if (!currentField) {
             
             

             
             let padding_length = 1;
             while (i + padding_length < this.size
                 && !this.offset_to_field_info[i + padding_length]) {
               ++padding_length;
             }

             
             struct.push(this._makePaddingField(padding_length));

             
             i += padding_length;
           } else {
             

             
             for (let j = 1; j < currentField.type.size; ++j) {
               let candidateField = this.offset_to_field_info[i + j];
               if (candidateField) {
                 throw new Error("Fields " + currentField.name +
                   " and " + candidateField.name +
                   " overlap at position " + (i + j));
               }
             }

             
             let field = ({});
             field[currentField.name] = currentField.type;
             struct.push(field);

             
             i += currentField.type.size;
           }
         }
         let result = new Type(this.name, ctypes.StructType(this.name, struct));
         if (result.implementation.size != this.size) {
           throw new Error("Wrong size for type " + this.name +
               ": expected " + this.size +
               ", found " + result.implementation.size +
               " (" + result.implementation.toSource() + ")");
         }
         return result;
       }
     };
     exports.OS.Shared.HollowStructure = HollowStructure;

     













        
        
     let declareFFI = function declareFFI(lib, symbol, abi,
                                          returnType ) {
       if (exports.OS.Shared.DEBUG) {
         LOG("Attempting to declare FFI ", symbol);
       }
       
       if (typeof symbol != "string") {
         throw new TypeError("declareFFI expects as first argument a string");
       }
       abi = abi || ctypes.default_abi;
       if (Object.prototype.toString.call(abi) != "[object CABI]") {
         
         
         throw new TypeError("declareFFI expects as second argument an abi or null");
       }
       if (!returnType.importFromC) {
         throw new TypeError("declareFFI expects as third argument an instance of Type");
       }
       let signature = [symbol, abi];
       let argtypes  = [];
       for (let i = 3; i < arguments.length; ++i) {
         let current = arguments[i];
         if (!current) {
           throw new TypeError("Missing type for argument " + ( i - 3 ) +
                               " of symbol " + symbol);
         }
         if (!current.implementation) {
           throw new TypeError("Missing implementation for argument " + (i - 3)
                               + " of symbol " + symbol
                               + " ( " + current.name + " )" );
         }
         signature.push(current.implementation);
       }
       try {
         let fun = lib.declare.apply(lib, signature);
         let result = function ffi() {
           let result = fun.apply(fun, arguments);
           return returnType.importFromC(result, symbol);
         };
         if (exports.OS.Shared.DEBUG) {
           result.fun = fun; 
         }
	 if (exports.OS.Shared.DEBUG) {
          LOG("Function", symbol, "declared");
	 }
         return result;
       } catch (x) {
         
         
	 if (exports.OS.Shared.DEBUG) {
          LOG("Could not declare function " + symbol, x);
	 }
         return null;
       }
     };
     exports.OS.Shared.declareFFI = declareFFI;

     
     let gOffsetByType;

     















     exports.OS.Shared.offsetBy =
       function offsetBy(pointer, length) {
         if (length === undefined || length < 0) {
           throw new TypeError("offsetBy expects a positive number");
         }
        if (!("isNull" in pointer)) {
           throw new TypeError("offsetBy expects a pointer");
         }
         if (length == 0) {
           return pointer;
         }
         let type = pointer.constructor;
         let size = type.targetType.size;
         if (size == 0 || size == null) {
           throw new TypeError("offsetBy cannot be applied to a pointer without size");
         }
         let bytes = length * size;
         if (!gOffsetByType || gOffsetByType.size <= bytes) {
           gOffsetByType = ctypes.uint8_t.array(bytes * 2);
         }
         let addr = ctypes.cast(pointer, gOffsetByType.ptr).
           contents.addressOfElement(bytes);
         return ctypes.cast(addr, type);
     };



   })(this);
}
