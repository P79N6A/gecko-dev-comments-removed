



{
  if (typeof Components != "undefined") {
    
    
    
    

    throw new Error("osfile_shared_allthreads.jsm cannot be used from the main thread yet");
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

     













     function Type(name, implementation, convert_from_c) {
       if (!(typeof name == "string")) {
         throw new TypeError("Type expects as first argument a name, got: "
                             + name);
       }
       if (!(implementation instanceof ctypes.CType)) {
         throw new TypeError("Type expects as second argument a ctypes.CType"+
                             ", got: " + implementation);
       }
       this.name = name;
       this.implementation = implementation;
       if (convert_from_c) {
         this.convert_from_c = convert_from_c;
       } else {
         this.convert_from_c = Type.prototype.convert_from_c;
       }
     }
     Type.prototype = {
       convert_from_c: function(value) {
         return value;
       },

       


       get in_ptr() {
         delete this.in_ptr;
         let ptr_t = new Type("[int] " + this.name + "*",
           this.implementation.ptr);
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
         let ptr_t = new Type("[out] " + this.name + "*",
           this.implementation.ptr);
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
         let ptr_t = new Type("[inout] " + this.name + "*",
           this.implementation.ptr);
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
         let type = new Type("[auto " + finalizer +"] " + this.name,
           this.implementation,
           function release(value, operation) {
             return ctypes.CDataFinalizer(
               parent.convert_from_c(value, operation),
               finalizer);
           });
         return type;
       },

       


       withName: function withName(name) {
         return Object.create(this, {name: {value: name}});
       }
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
       if (!type.size) {
         throw new TypeError("Argument is not a proper C type");
       }
       LOG("Determining best projection for", type,
             "(size: ", type.size, ")", signed?"signed":"unsigned");
       
       if (type.size == 8           
           || type == ctypes.size_t 
           || type == ctypes.ssize_t
           || type == ctypes.intptr_t
           || type == ctypes.uintptr_t
           || type == ctypes.off_t){
          if (signed) {
            LOG("Projected as a large signed integer");
            return projectLargeInt;
          } else {
            LOG("Projected as a large unsigned integer");
            return projectLargeUInt;
          }
       }
       LOG("Projected as a regular number");
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
       new Type("void*",
                ctypes.voidptr_t);

     
     
     
     ["in_ptr", "out_ptr", "inout_ptr"].forEach(function(key) {
       Object.defineProperty(Types.void_t, key,
       {
         get: function() {
           return Types.voidptr_t;
         }
       });
     });

     


     Types.char =
       new Type("char",
                ctypes.char);

     


     Types.jschar =
       new Type("jschar",
                ctypes.jschar);

     




     Types.int =
       new Type("int",
                ctypes.int,
                projector(ctypes.int, true));

     Types.unsigned_int =
       new Type("unsigned int",
                ctypes.unsigned_int,
                projector(ctypes.unsigned_int, false));

     


     Types.int8_t =
       new Type("int8_t",
                ctypes.int8_t,
                projectValue);

     Types.uint8_t =
       new Type("uint8_t",
                ctypes.uint8_t,
                projectValue);

     




     Types.int16_t =
       new Type("int16_t",
                ctypes.int16_t,
                projectValue);

     Types.uint16_t =
       new Type("uint16_t",
                ctypes.uint16_t,
                projectValue);

     




     Types.int32_t =
       new Type("int32_t",
                ctypes.int32_t,
                projectValue);

     Types.uint32_t =
       new Type("uint32_t",
                ctypes.uint32_t,
                projectValue);

     


     Types.int64_t =
       new Type("int64_t",
                ctypes.int64_t,
                projectLargeInt);

     Types.uint64_t =
       new Type("uint64_t",
                ctypes.uint64_t,
                projectLargeUInt);

     



     Types.long =
       new Type("long",
                ctypes.long,
                projector(ctypes.long, true));

     



     Types.bool =
       new Type("bool",
                ctypes.int,
                function projectBool(x) {
                  return !!(x.value);
                });

     



     Types.uid_t =
       new Type("uid_t",
                ctypes.int,
                projector(ctypes.int, true));

     



     Types.gid_t =
       new Type("gid_t",
                ctypes.int,
                projector(ctypes.int, true));

     



     Types.off_t =
       new Type("off_t",
                ctypes.off_t,
                projector(ctypes.off_t, true));

     



     Types.size_t =
       new Type("size_t",
                ctypes.size_t,
                projector(ctypes.size_t, false));

     



     Types.ssize_t =
       new Type("ssize_t",
                ctypes.ssize_t,
                projector(ctypes.ssize_t, true));


     






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
       LOG("Attempting to declare FFI ", symbol);
       
       if (typeof symbol != "string") {
         throw new TypeError("declareFFI expects as first argument a string");
       }
       abi = abi || ctypes.default_abi;
       if (Object.prototype.toString.call(abi) != "[object CABI]") {
         
         
         throw new TypeError("declareFFI expects as second argument an abi or null");
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
           return returnType.convert_from_c(result, symbol);
         };
         if (exports.OS.Shared.DEBUG) {
           result.fun = fun; 
         }
         LOG("Function", symbol, "declared");
         return result;
       } catch (x) {
         
         
         LOG("Could not declare function " + symbol, x);
         return null;
       }
     };
     exports.OS.Shared.declareFFI = declareFFI;


     


     let _aux = {};
     exports.OS.Shared._aux = _aux;

     














     _aux.normalizeOpenMode = function normalizeOpenMode(mode) {
       let result = {
         read: false,
         write: false,
         trunc: false,
         create: false,
         existing: false
       };
       for (let key in mode) {
         if (!mode[key]) continue; 
         switch (key) {
         case "read":
           result.read = true;
           break;
         case "write":
           result.write = true;
           break;
         case "truncate": 
         case "trunc":
           result.trunc = true;
           result.write = true;
           break;
         case "create":
           result.create = true;
           result.write = true;
           break;
         case "existing": 
         case "exist":
           result.existing = true;
           break;
         default:
           throw new TypeError("Mode " + key + " not understood");
         }
       }
       
       if (result.existing && result.create) {
         throw new TypeError("Cannot specify both existing:true and create:true");
       }
       if (result.trunc && result.create) {
         throw new TypeError("Cannot specify both trunc:true and create:true");
       }
       
       if (!result.write) {
         result.read = true;
       }
       return result;
     };
   })(this);
}
