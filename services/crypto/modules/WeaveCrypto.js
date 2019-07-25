




































const EXPORTED_SYMBOLS = ["WeaveCrypto"];

const Cc = Components.classes;
const Ci = Components.interfaces;
const Cr = Components.results;

Components.utils.import("resource://gre/modules/XPCOMUtils.jsm");
Components.utils.import("resource://gre/modules/Services.jsm");
Components.utils.import("resource://gre/modules/ctypes.jsm");

function WeaveCrypto() {
    this.init();
}

WeaveCrypto.prototype = {
    QueryInterface: XPCOMUtils.generateQI([Ci.IWeaveCrypto]),

    prefBranch : null,
    debug      : true,  
    nss        : null,
    nss_t      : null,

    observer : {
        _self : null,

        QueryInterface : XPCOMUtils.generateQI([Ci.nsIObserver,
                                                Ci.nsISupportsWeakReference]),

        observe : function (subject, topic, data) {
            let self = this._self;
            self.log("Observed " + topic + " topic.");
            if (topic == "nsPref:changed") {
                self.debug = self.prefBranch.getBoolPref("cryptoDebug");
            }
        }
    },

    init : function() {
        try {
            
            this.prefBranch = Services.prefs.getBranch("services.sync.log.");
            this.prefBranch.QueryInterface(Ci.nsIPrefBranch2);
            this.prefBranch.addObserver("cryptoDebug", this.observer, false);
            this.observer._self = this;
            this.debug = this.prefBranch.getBoolPref("cryptoDebug");

            this.initNSS();
        } catch (e) {
            this.log("init failed: " + e);
            throw e;
        }
    },

    log : function (message) {
        if (!this.debug)
            return;
        dump("WeaveCrypto: " + message + "\n");
        Services.console.logStringMessage("WeaveCrypto: " + message);
    },

    initNSS : function() {
        
        
        
        
        Cc["@mozilla.org/psm;1"].getService(Ci.nsISupports);

        
        let path = ctypes.libraryName("nss3");

        
        var nsslib;
        try {
            this.log("Trying NSS library without path");
            nsslib = ctypes.open(path);
        } catch(e) {
            
            
            let file = Services.dirsvc.get("GreD", Ci.nsILocalFile);
            file.append(path);
            this.log("Trying again with path " + file.path);
            nsslib = ctypes.open(file.path);
        }

        this.log("Initializing NSS types and function declarations...");

        this.nss = {};
        this.nss_t = {};

        
        
        this.nss_t.PRBool = ctypes.int;
        
        
        this.nss_t.SECStatus = ctypes.int;
        
        
        this.nss_t.PK11SlotInfo = ctypes.void_t;
        
        this.nss_t.CK_MECHANISM_TYPE = ctypes.unsigned_long;
        this.nss_t.CK_ATTRIBUTE_TYPE = ctypes.unsigned_long;
        this.nss_t.CK_KEY_TYPE       = ctypes.unsigned_long;
        this.nss_t.CK_OBJECT_HANDLE  = ctypes.unsigned_long;
        
        
        this.nss_t.PK11Origin = ctypes.int;
        
        this.nss.PK11_OriginUnwrap = 4;
        
        
        this.nss_t.PK11SymKey = ctypes.void_t;
        
        
        this.nss_t.SECOidTag = ctypes.int;
        
        
        this.nss_t.SECItemType = ctypes.int;
        
        this.nss.SIBUFFER = 0;
        
        
        this.nss_t.PK11Context = ctypes.void_t;
        
        this.nss_t.PLArenaPool = ctypes.void_t;
        
        
        this.nss_t.KeyType = ctypes.int;
        
        
        this.nss_t.PK11AttrFlags = ctypes.unsigned_int;
        
        
        this.nss_t.SECOidTag = ctypes.int;
        
        
        this.nss_t.SECItem = ctypes.StructType(
            "SECItem", [{ type: this.nss_t.SECItemType },
                        { data: ctypes.unsigned_char.ptr },
                        { len : ctypes.int }]);
        
        
        this.nss_t.PK11RSAGenParams = ctypes.StructType(
            "PK11RSAGenParams", [{ keySizeInBits: ctypes.int },
                                 { pe : ctypes.unsigned_long }]);
        
        
        this.nss_t.SECKEYPrivateKey = ctypes.StructType(
            "SECKEYPrivateKey", [{ arena:        this.nss_t.PLArenaPool.ptr  },
                                 { keyType:      this.nss_t.KeyType          },
                                 { pkcs11Slot:   this.nss_t.PK11SlotInfo.ptr },
                                 { pkcs11ID:     this.nss_t.CK_OBJECT_HANDLE },
                                 { pkcs11IsTemp: this.nss_t.PRBool           },
                                 { wincx:        ctypes.voidptr_t            },
                                 { staticflags:  ctypes.unsigned_int         }]);
        
        
        this.nss_t.SECKEYRSAPublicKey = ctypes.StructType(
            "SECKEYRSAPublicKey", [{ arena:          this.nss_t.PLArenaPool.ptr },
                                   { modulus:        this.nss_t.SECItem         },
                                   { publicExponent: this.nss_t.SECItem         }]);
        
        
        this.nss_t.SECKEYPublicKey = ctypes.StructType(
            "SECKEYPublicKey", [{ arena:      this.nss_t.PLArenaPool.ptr    },
                                { keyType:    this.nss_t.KeyType            },
                                { pkcs11Slot: this.nss_t.PK11SlotInfo.ptr   },
                                { pkcs11ID:   this.nss_t.CK_OBJECT_HANDLE   },
                                { rsa:        this.nss_t.SECKEYRSAPublicKey } ]);
                                
                                
                                
                                
                                
                                
        
        
        this.nss_t.SECAlgorithmID = ctypes.StructType(
            "SECAlgorithmID", [{ algorithm:  this.nss_t.SECItem },
                               { parameters: this.nss_t.SECItem }]);
        
        
        this.nss_t.CERTSubjectPublicKeyInfo = ctypes.StructType(
            "CERTSubjectPublicKeyInfo", [{ arena:            this.nss_t.PLArenaPool.ptr },
                                         { algorithm:        this.nss_t.SECAlgorithmID  },
                                         { subjectPublicKey: this.nss_t.SECItem         }]);


        
        this.nss.CKK_RSA = 0x0;
        this.nss.CKM_RSA_PKCS_KEY_PAIR_GEN = 0x0000;
        this.nss.CKM_AES_KEY_GEN           = 0x1080; 
        this.nss.CKA_ENCRYPT = 0x104;
        this.nss.CKA_DECRYPT = 0x105;
        this.nss.CKA_UNWRAP  = 0x107;

        
        this.nss.PK11_ATTR_SESSION   = 0x02;
        this.nss.PK11_ATTR_PUBLIC    = 0x08;
        this.nss.PK11_ATTR_SENSITIVE = 0x40;

        
        this.nss.SEC_OID_PKCS5_PBKDF2         = 291;
        this.nss.SEC_OID_HMAC_SHA1            = 294;
        this.nss.SEC_OID_PKCS1_RSA_ENCRYPTION = 16;


        
        
        this.nss.PK11_GenerateRandom = nsslib.declare("PK11_GenerateRandom",
                                                      ctypes.default_abi, this.nss_t.SECStatus,
                                                      ctypes.unsigned_char.ptr, ctypes.int);
        
        
        this.nss.PK11_GetInternalSlot = nsslib.declare("PK11_GetInternalSlot",
                                                       ctypes.default_abi, this.nss_t.PK11SlotInfo.ptr);
        
        
        this.nss.PK11_GetInternalKeySlot = nsslib.declare("PK11_GetInternalKeySlot",
                                                          ctypes.default_abi, this.nss_t.PK11SlotInfo.ptr);
        
        
        this.nss.PK11_KeyGen = nsslib.declare("PK11_KeyGen",
                                              ctypes.default_abi, this.nss_t.PK11SymKey.ptr,
                                              this.nss_t.PK11SlotInfo.ptr, this.nss_t.CK_MECHANISM_TYPE,
                                              this.nss_t.SECItem.ptr, ctypes.int, ctypes.voidptr_t);
        
        
        this.nss.PK11_ExtractKeyValue = nsslib.declare("PK11_ExtractKeyValue",
                                                       ctypes.default_abi, this.nss_t.SECStatus,
                                                       this.nss_t.PK11SymKey.ptr);
        
        
        this.nss.PK11_GetKeyData = nsslib.declare("PK11_GetKeyData",
                                                  ctypes.default_abi, this.nss_t.SECItem.ptr,
                                                  this.nss_t.PK11SymKey.ptr);
        
        
        this.nss.PK11_AlgtagToMechanism = nsslib.declare("PK11_AlgtagToMechanism",
                                                         ctypes.default_abi, this.nss_t.CK_MECHANISM_TYPE,
                                                         this.nss_t.SECOidTag);
        
        
        this.nss.PK11_GetIVLength = nsslib.declare("PK11_GetIVLength",
                                                   ctypes.default_abi, ctypes.int,
                                                   this.nss_t.CK_MECHANISM_TYPE);
        
        
        this.nss.PK11_GetBlockSize = nsslib.declare("PK11_GetBlockSize",
                                                    ctypes.default_abi, ctypes.int,
                                                    this.nss_t.CK_MECHANISM_TYPE, this.nss_t.SECItem.ptr);
        
        
        this.nss.PK11_GetPadMechanism = nsslib.declare("PK11_GetPadMechanism",
                                                       ctypes.default_abi, this.nss_t.CK_MECHANISM_TYPE,
                                                       this.nss_t.CK_MECHANISM_TYPE);
        
        
        this.nss.PK11_ParamFromIV = nsslib.declare("PK11_ParamFromIV",
                                                   ctypes.default_abi, this.nss_t.SECItem.ptr,
                                                   this.nss_t.CK_MECHANISM_TYPE, this.nss_t.SECItem.ptr);
        
        
        
        this.nss.PK11_ImportSymKey = nsslib.declare("PK11_ImportSymKey",
                                                    ctypes.default_abi, this.nss_t.PK11SymKey.ptr,
                                                    this.nss_t.PK11SlotInfo.ptr, this.nss_t.CK_MECHANISM_TYPE, this.nss_t.PK11Origin,
                                                    this.nss_t.CK_ATTRIBUTE_TYPE, this.nss_t.SECItem.ptr, ctypes.voidptr_t);
        
        
        
        this.nss.PK11_CreateContextBySymKey = nsslib.declare("PK11_CreateContextBySymKey",
                                                             ctypes.default_abi, this.nss_t.PK11Context.ptr,
                                                             this.nss_t.CK_MECHANISM_TYPE, this.nss_t.CK_ATTRIBUTE_TYPE,
                                                             this.nss_t.PK11SymKey.ptr, this.nss_t.SECItem.ptr);
        
        
        
        this.nss.PK11_CipherOp = nsslib.declare("PK11_CipherOp",
                                                ctypes.default_abi, this.nss_t.SECStatus,
                                                this.nss_t.PK11Context.ptr, ctypes.unsigned_char.ptr,
                                                ctypes.int.ptr, ctypes.int, ctypes.unsigned_char.ptr, ctypes.int);
        
        
        
        this.nss.PK11_DigestFinal = nsslib.declare("PK11_DigestFinal",
                                                   ctypes.default_abi, this.nss_t.SECStatus,
                                                   this.nss_t.PK11Context.ptr, ctypes.unsigned_char.ptr,
                                                   ctypes.unsigned_int.ptr, ctypes.unsigned_int);
        
        
        
        
        this.nss.PK11_GenerateKeyPairWithFlags = nsslib.declare("PK11_GenerateKeyPairWithFlags",
                                                   ctypes.default_abi, this.nss_t.SECKEYPrivateKey.ptr,
                                                   this.nss_t.PK11SlotInfo.ptr, this.nss_t.CK_MECHANISM_TYPE, ctypes.voidptr_t,
                                                   this.nss_t.SECKEYPublicKey.ptr.ptr, this.nss_t.PK11AttrFlags, ctypes.voidptr_t);
        
        
        this.nss.PK11_SetPrivateKeyNickname = nsslib.declare("PK11_SetPrivateKeyNickname",
                                                             ctypes.default_abi, this.nss_t.SECStatus,
                                                             this.nss_t.SECKEYPrivateKey.ptr, ctypes.char.ptr);
        
        
        
        
        this.nss.PK11_CreatePBEV2AlgorithmID = nsslib.declare("PK11_CreatePBEV2AlgorithmID",
                                                              ctypes.default_abi, this.nss_t.SECAlgorithmID.ptr,
                                                              this.nss_t.SECOidTag, this.nss_t.SECOidTag, this.nss_t.SECOidTag, 
                                                              ctypes.int, ctypes.int, this.nss_t.SECItem.ptr);
        
        
        this.nss.PK11_PBEKeyGen = nsslib.declare("PK11_PBEKeyGen",
                                                 ctypes.default_abi, this.nss_t.PK11SymKey.ptr,
                                                 this.nss_t.PK11SlotInfo.ptr, this.nss_t.SECAlgorithmID.ptr,
                                                 this.nss_t.SECItem.ptr, this.nss_t.PRBool, ctypes.voidptr_t);
        
        
        
        
        this.nss.PK11_WrapPrivKey = nsslib.declare("PK11_WrapPrivKey",
                                                   ctypes.default_abi, this.nss_t.SECStatus,
                                                   this.nss_t.PK11SlotInfo.ptr, this.nss_t.PK11SymKey.ptr,
                                                   this.nss_t.SECKEYPrivateKey.ptr, this.nss_t.CK_MECHANISM_TYPE,
                                                   this.nss_t.SECItem.ptr, this.nss_t.SECItem.ptr, ctypes.voidptr_t);
        
        
        this.nss.SECKEY_EncodeDERSubjectPublicKeyInfo = nsslib.declare("SECKEY_EncodeDERSubjectPublicKeyInfo",
                                                                       ctypes.default_abi, this.nss_t.SECItem.ptr,
                                                                       this.nss_t.SECKEYPublicKey.ptr);
        
        
        this.nss.SECKEY_DecodeDERSubjectPublicKeyInfo = nsslib.declare("SECKEY_DecodeDERSubjectPublicKeyInfo",
                                                                       ctypes.default_abi, this.nss_t.CERTSubjectPublicKeyInfo.ptr,
                                                                       this.nss_t.SECItem.ptr);
        
        
        this.nss.SECKEY_ExtractPublicKey = nsslib.declare("SECKEY_ExtractPublicKey",
                                                          ctypes.default_abi, this.nss_t.SECKEYPublicKey.ptr,
                                                          this.nss_t.CERTSubjectPublicKeyInfo.ptr);
        
        
        
        this.nss.PK11_PubWrapSymKey = nsslib.declare("PK11_PubWrapSymKey",
                                                     ctypes.default_abi, this.nss_t.SECStatus,
                                                     this.nss_t.CK_MECHANISM_TYPE, this.nss_t.SECKEYPublicKey.ptr,
                                                     this.nss_t.PK11SymKey.ptr, this.nss_t.SECItem.ptr);
        
        
        
        
        
        
        
        this.nss.PK11_UnwrapPrivKey = nsslib.declare("PK11_UnwrapPrivKey",
                                                     ctypes.default_abi, this.nss_t.SECKEYPrivateKey.ptr,
                                                     this.nss_t.PK11SlotInfo.ptr, this.nss_t.PK11SymKey.ptr,
                                                     this.nss_t.CK_MECHANISM_TYPE, this.nss_t.SECItem.ptr,
                                                     this.nss_t.SECItem.ptr, this.nss_t.SECItem.ptr,
                                                     this.nss_t.SECItem.ptr, this.nss_t.PRBool,
                                                     this.nss_t.PRBool, this.nss_t.CK_KEY_TYPE,
                                                     this.nss_t.CK_ATTRIBUTE_TYPE.ptr, ctypes.int,
                                                     ctypes.voidptr_t);
        
        
        
        this.nss.PK11_PubUnwrapSymKey = nsslib.declare("PK11_PubUnwrapSymKey",
                                                       ctypes.default_abi, this.nss_t.PK11SymKey.ptr,
                                                       this.nss_t.SECKEYPrivateKey.ptr, this.nss_t.SECItem.ptr,
                                                       this.nss_t.CK_MECHANISM_TYPE, this.nss_t.CK_ATTRIBUTE_TYPE, ctypes.int);
        
        
        this.nss.PK11_DestroyContext = nsslib.declare("PK11_DestroyContext",
                                                       ctypes.default_abi, ctypes.void_t,
                                                       this.nss_t.PK11Context.ptr, this.nss_t.PRBool);
        
        
        this.nss.PK11_FreeSymKey = nsslib.declare("PK11_FreeSymKey",
                                                  ctypes.default_abi, ctypes.void_t,
                                                  this.nss_t.PK11SymKey.ptr);
        
        
        this.nss.PK11_FreeSlot = nsslib.declare("PK11_FreeSlot",
                                                ctypes.default_abi, ctypes.void_t,
                                                this.nss_t.PK11SlotInfo.ptr);
        
        
        this.nss.SECITEM_FreeItem = nsslib.declare("SECITEM_FreeItem",
                                                   ctypes.default_abi, ctypes.void_t,
                                                   this.nss_t.SECItem.ptr, this.nss_t.PRBool);
        
        
        this.nss.SECKEY_DestroyPublicKey = nsslib.declare("SECKEY_DestroyPublicKey",
                                                          ctypes.default_abi, ctypes.void_t,
                                                          this.nss_t.SECKEYPublicKey.ptr);
        
        
        this.nss.SECKEY_DestroyPrivateKey = nsslib.declare("SECKEY_DestroyPrivateKey",
                                                           ctypes.default_abi, ctypes.void_t,
                                                           this.nss_t.SECKEYPrivateKey.ptr);
        
        
        this.nss.SECOID_DestroyAlgorithmID = nsslib.declare("SECOID_DestroyAlgorithmID",
                                                            ctypes.default_abi, ctypes.void_t,
                                                            this.nss_t.SECAlgorithmID.ptr, this.nss_t.PRBool);
        
        
        this.nss.SECKEY_DestroySubjectPublicKeyInfo = nsslib.declare("SECKEY_DestroySubjectPublicKeyInfo",
                                                                     ctypes.default_abi, ctypes.void_t,
                                                                     this.nss_t.CERTSubjectPublicKeyInfo.ptr);
    },


    
    
    


    algorithm : Ci.IWeaveCrypto.AES_256_CBC,

    encrypt : function(clearTextUCS2, symmetricKey, iv) {
        this.log("encrypt() called");

        
        
        let inputBuffer = new ctypes.ArrayType(ctypes.unsigned_char)(clearTextUCS2);
        inputBuffer = ctypes.cast(inputBuffer, ctypes.unsigned_char.array(inputBuffer.length - 1));

        
        
        
        let mech = this.nss.PK11_AlgtagToMechanism(this.algorithm);
        let blockSize = this.nss.PK11_GetBlockSize(mech, null);
        let outputBufferSize = inputBuffer.length + blockSize;
        let outputBuffer = new ctypes.ArrayType(ctypes.unsigned_char, outputBufferSize)();

        outputBuffer = this._commonCrypt(inputBuffer, outputBuffer, symmetricKey, iv, this.nss.CKA_ENCRYPT);

        return this.encodeBase64(outputBuffer.address(), outputBuffer.length);
    },


    decrypt : function(cipherText, symmetricKey, iv) {
        this.log("decrypt() called");

        let inputUCS2 = "";
        if (cipherText.length)
            inputUCS2 = atob(cipherText);

        
        
        
        let input = new ctypes.ArrayType(ctypes.unsigned_char, inputUCS2.length)();
        this.byteCompress(inputUCS2, input);

        let outputBuffer = new ctypes.ArrayType(ctypes.unsigned_char, input.length)();

        outputBuffer = this._commonCrypt(input, outputBuffer, symmetricKey, iv, this.nss.CKA_DECRYPT);

        
        
        
        return "" + outputBuffer.readString() + "";
    },


    _commonCrypt : function (input, output, symmetricKey, iv, operation) {
        this.log("_commonCrypt() called");
        
        let keyItem = this.makeSECItem(symmetricKey, true);
        let ivItem  = this.makeSECItem(iv, true);

        
        
        let mechanism = this.nss.PK11_AlgtagToMechanism(this.algorithm);
        mechanism = this.nss.PK11_GetPadMechanism(mechanism);
        if (mechanism == this.nss.CKM_INVALID_MECHANISM)
            throw Components.Exception("invalid algorithm (can't pad)", Cr.NS_ERROR_FAILURE);

        let ctx, symKey, slot, ivParam;
        try {
            ivParam = this.nss.PK11_ParamFromIV(mechanism, ivItem.address());
            if (ivParam.isNull())
                throw Components.Exception("can't convert IV to param", Cr.NS_ERROR_FAILURE);

            slot = this.nss.PK11_GetInternalKeySlot();
            if (slot.isNull())
                throw Components.Exception("can't get internal key slot", Cr.NS_ERROR_FAILURE);

            symKey = this.nss.PK11_ImportSymKey(slot, mechanism, this.nss.PK11_OriginUnwrap, operation, keyItem.address(), null);
            if (symKey.isNull())
                throw Components.Exception("symkey import failed", Cr.NS_ERROR_FAILURE);

            ctx = this.nss.PK11_CreateContextBySymKey(mechanism, operation, symKey, ivParam);
            if (ctx.isNull())
                throw Components.Exception("couldn't create context for symkey", Cr.NS_ERROR_FAILURE);

            let maxOutputSize = output.length;
            let tmpOutputSize = new ctypes.int(); 

            if (this.nss.PK11_CipherOp(ctx, output, tmpOutputSize.address(), maxOutputSize, input, input.length))
                throw Components.Exception("cipher operation failed", Cr.NS_ERROR_FAILURE);

            let actualOutputSize = tmpOutputSize.value;
            let finalOutput = output.addressOfElement(actualOutputSize);
            maxOutputSize -= actualOutputSize;

            
            
            
            let tmpOutputSize2 = new ctypes.unsigned_int(); 
            if (this.nss.PK11_DigestFinal(ctx, finalOutput, tmpOutputSize2.address(), maxOutputSize))
                throw Components.Exception("cipher finalize failed", Cr.NS_ERROR_FAILURE);

            actualOutputSize += tmpOutputSize2.value;
            let newOutput = ctypes.cast(output, ctypes.unsigned_char.array(actualOutputSize));
            return newOutput;
        } catch (e) {
            this.log("_commonCrypt: failed: " + e);
            throw e;
        } finally {
            if (ctx && !ctx.isNull())
                this.nss.PK11_DestroyContext(ctx, true);
            if (symKey && !symKey.isNull())
                this.nss.PK11_FreeSymKey(symKey);
            if (slot && !slot.isNull())
                this.nss.PK11_FreeSlot(slot);
            if (ivParam && !ivParam.isNull())
                this.nss.SECITEM_FreeItem(ivParam, true);
        }
    },


    generateRandomKey : function() {
        this.log("generateRandomKey() called");
        let encodedKey, keygenMech, keySize;

        
        switch(this.algorithm) {
          case Ci.IWeaveCrypto.AES_128_CBC:
            keygenMech = this.nss.CKM_AES_KEY_GEN;
            keySize = 16;
            break;

          case Ci.IWeaveCrypto.AES_192_CBC:
            keygenMech = this.nss.CKM_AES_KEY_GEN;
            keySize = 24;
            break;

          case Ci.IWeaveCrypto.AES_256_CBC:
            keygenMech = this.nss.CKM_AES_KEY_GEN;
            keySize = 32;
            break;

          default:
            throw Components.Exception("unknown algorithm", Cr.NS_ERROR_FAILURE);
        }

        let slot, randKey, keydata;
        try {
            slot = this.nss.PK11_GetInternalSlot();
            if (slot.isNull())
                throw Components.Exception("couldn't get internal slot", Cr.NS_ERROR_FAILURE);

            randKey = this.nss.PK11_KeyGen(slot, keygenMech, null, keySize, null);
            if (randKey.isNull())
                throw Components.Exception("PK11_KeyGen failed.", Cr.NS_ERROR_FAILURE);

            
            
            if (this.nss.PK11_ExtractKeyValue(randKey))
                throw Components.Exception("PK11_ExtractKeyValue failed.", Cr.NS_ERROR_FAILURE);

            keydata = this.nss.PK11_GetKeyData(randKey);
            if (keydata.isNull())
                throw Components.Exception("PK11_GetKeyData failed.", Cr.NS_ERROR_FAILURE);

            return this.encodeBase64(keydata.contents.data, keydata.contents.len);
        } catch (e) {
            this.log("generateRandomKey: failed: " + e);
            throw e;
        } finally {
            if (randKey && !randKey.isNull())
                this.nss.PK11_FreeSymKey(randKey);
            if (slot && !slot.isNull())
                this.nss.PK11_FreeSlot(slot);
        }
    },


    generateRandomIV : function() {
        this.log("generateRandomIV() called");

        let mech = this.nss.PK11_AlgtagToMechanism(this.algorithm);
        let size = this.nss.PK11_GetIVLength(mech);

        return this.generateRandomBytes(size);
    },


    generateRandomBytes : function(byteCount) {
        this.log("generateRandomBytes() called");

        
        let scratch = new ctypes.ArrayType(ctypes.unsigned_char, byteCount)();
        if (this.nss.PK11_GenerateRandom(scratch, byteCount))
            throw Components.Exception("PK11_GenrateRandom failed", Cr.NS_ERROR_FAILURE);

        return this.encodeBase64(scratch.address(), scratch.length);
    },


    
    
    


    
    
    byteCompress : function (jsString, charArray) {
        let intArray = ctypes.cast(charArray, ctypes.uint8_t.array(charArray.length));
        for (let i = 0; i < jsString.length; i++)
            intArray[i] = jsString.charCodeAt(i) % 256; 
    },

    
    
    byteExpand : function (charArray) {
        let expanded = "";
        let len = charArray.length;
        let intData = ctypes.cast(charArray, ctypes.uint8_t.array(len));
        for (let i = 0; i < len; i++)
            expanded += String.fromCharCode(intData[i]);
        return expanded;
    },

    expandData : function expandData(data, len) {
        
        
        let expanded = "";
        let intData = ctypes.cast(data, ctypes.uint8_t.array(len).ptr).contents;
        for (let i = 0; i < len; i++)
            expanded += String.fromCharCode(intData[i]);
      return expanded;
    },

    encodeBase64 : function (data, len) {
        return btoa(this.expandData(data, len));
    },

    makeSECItem : function(input, isEncoded) {
        if (isEncoded)
            input = atob(input);
        let outputData = new ctypes.ArrayType(ctypes.unsigned_char, input.length)();
        this.byteCompress(input, outputData);

        return new this.nss_t.SECItem(this.nss.SIBUFFER, outputData, outputData.length);
    },


    


    deriveKeyFromPassphrase : function deriveKeyFromPassphrase(passphrase, salt, keyLength) {
        this.log("deriveKeyFromPassphrase() called.");
        let passItem = this.makeSECItem(passphrase, false);
        let saltItem = this.makeSECItem(salt, true);

        let pbeAlg = this.algorithm;
        let cipherAlg = this.algorithm; 
        let prfAlg = this.nss.SEC_OID_HMAC_SHA1; 

        let keyLength  = keyLength || 0;    
        let iterations = 4096; 

        let algid, slot, symKey, keyData;
        try {
            algid = this.nss.PK11_CreatePBEV2AlgorithmID(pbeAlg, cipherAlg, prfAlg,
                                                        keyLength, iterations, saltItem.address());
            if (algid.isNull())
                throw Components.Exception("PK11_CreatePBEV2AlgorithmID failed", Cr.NS_ERROR_FAILURE);

            slot = this.nss.PK11_GetInternalSlot();
            if (slot.isNull())
                throw Components.Exception("couldn't get internal slot", Cr.NS_ERROR_FAILURE);

            symKey = this.nss.PK11_PBEKeyGen(slot, algid, passItem.address(), false, null);
            if (symKey.isNull())
                throw Components.Exception("PK11_PBEKeyGen failed", Cr.NS_ERROR_FAILURE);

            
            if (this.nss.PK11_ExtractKeyValue(symKey)) {
                throw this.makeException("PK11_ExtractKeyValue failed.", Cr.NS_ERROR_FAILURE);
            }

            keyData = this.nss.PK11_GetKeyData(symKey);

            if (keyData.isNull())
                throw Components.Exception("PK11_GetKeyData failed", Cr.NS_ERROR_FAILURE);

            
            
            return this.expandData(keyData.contents.data, keyData.contents.len);

        } catch (e) {
            this.log("deriveKeyFromPassphrase: failed: " + e);
            throw e;
        } finally {
            if (algid && !algid.isNull())
                this.nss.SECOID_DestroyAlgorithmID(algid, true);
            if (slot && !slot.isNull())
                this.nss.PK11_FreeSlot(slot);
            if (symKey && !symKey.isNull())
                this.nss.PK11_FreeSymKey(symKey);
    }
    },
};
