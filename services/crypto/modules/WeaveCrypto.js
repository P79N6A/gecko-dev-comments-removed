




































const EXPORTED_SYMBOLS = ["WeaveCrypto"];

const Cc = Components.classes;
const Ci = Components.interfaces;
const Cr = Components.results;

Components.utils.import("resource://gre/modules/XPCOMUtils.jsm");
Components.utils.import("resource://gre/modules/Services.jsm");
Components.utils.import("resource://gre/modules/ctypes.jsm");

const ALGORITHM                 = Ci.IWeaveCrypto.AES_256_CBC;
const KEYSIZE_AES_256           = 32;
const KEY_DERIVATION_ITERATIONS = 4096;   

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
            this.initAlgorithmSettings();   
            this.initIVSECItem();
            this.initSharedInts();
        } catch (e) {
            this.log("init failed: " + e);
            throw e;
        }
    },

    
    _commonCryptSignedOutputSize:       null,
    _commonCryptSignedOutputSizeAddr:   null,
    _commonCryptUnsignedOutputSize:     null,
    _commonCryptUnsignedOutputSizeAddr: null,

    initSharedInts: function initSharedInts() {
        let signed   = new ctypes.int();
        let unsigned = new ctypes.unsigned_int();
        this._commonCryptSignedOutputSize       = signed;
        this._commonCryptUnsignedOutputSize     = unsigned;
        this._commonCryptSignedOutputSizeAddr   = signed.address();
        this._commonCryptUnsignedOutputSizeAddr = unsigned.address();
    },

    









    initAlgorithmSettings: function() {
        this.mechanism = this.nss.PK11_AlgtagToMechanism(ALGORITHM);
        this.blockSize = this.nss.PK11_GetBlockSize(this.mechanism, null);
        this.ivLength  = this.nss.PK11_GetIVLength(this.mechanism);
        this.keySize   = KEYSIZE_AES_256;
        this.keygenMechanism = this.nss.CKM_AES_KEY_GEN;  

        
        
        this.padMechanism = this.nss.PK11_GetPadMechanism(this.mechanism);
        if (this.padMechanism == this.nss.CKM_INVALID_MECHANISM)
            throw Components.Exception("invalid algorithm (can't pad)", Cr.NS_ERROR_FAILURE);
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
        
        
        this.nss_t.SECItem = ctypes.StructType(
            "SECItem", [{ type: this.nss_t.SECItemType },
                        { data: ctypes.unsigned_char.ptr },
                        { len : ctypes.int }]);
        
        
        this.nss_t.SECAlgorithmID = ctypes.StructType(
            "SECAlgorithmID", [{ algorithm:  this.nss_t.SECItem },
                               { parameters: this.nss_t.SECItem }]);


        
        this.nss.CKK_RSA = 0x0;
        this.nss.CKM_RSA_PKCS_KEY_PAIR_GEN = 0x0000;
        this.nss.CKM_AES_KEY_GEN           = 0x1080; 
        this.nss.CKA_ENCRYPT = 0x104;
        this.nss.CKA_DECRYPT = 0x105;

        
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
        
        
        
        
        this.nss.PK11_CreatePBEV2AlgorithmID = nsslib.declare("PK11_CreatePBEV2AlgorithmID",
                                                              ctypes.default_abi, this.nss_t.SECAlgorithmID.ptr,
                                                              this.nss_t.SECOidTag, this.nss_t.SECOidTag, this.nss_t.SECOidTag, 
                                                              ctypes.int, ctypes.int, this.nss_t.SECItem.ptr);
        
        
        this.nss.PK11_PBEKeyGen = nsslib.declare("PK11_PBEKeyGen",
                                                 ctypes.default_abi, this.nss_t.PK11SymKey.ptr,
                                                 this.nss_t.PK11SlotInfo.ptr, this.nss_t.SECAlgorithmID.ptr,
                                                 this.nss_t.SECItem.ptr, this.nss_t.PRBool, ctypes.voidptr_t);
        
        
        this.nss.PK11_DestroyContext = nsslib.declare("PK11_DestroyContext",
                                                       ctypes.default_abi, ctypes.void_t,
                                                       this.nss_t.PK11Context.ptr, this.nss_t.PRBool);
        
        
        this.nss.PK11_FreeSymKey = nsslib.declare("PK11_FreeSymKey",
                                                  ctypes.default_abi, ctypes.void_t,
                                                  this.nss_t.PK11SymKey.ptr);
        
        
        this.nss.PK11_FreeSlot = nsslib.declare("PK11_FreeSlot",
                                                ctypes.default_abi, ctypes.void_t,
                                                this.nss_t.PK11SlotInfo.ptr);
        
        
        this.nss.SECITEM_AllocItem = nsslib.declare("SECITEM_AllocItem",
                                                    ctypes.default_abi, this.nss_t.SECItem.ptr,
                                                    this.nss_t.PLArenaPool.ptr,     
                                                    this.nss_t.SECItem.ptr, ctypes.unsigned_int);
        
        
        this.nss.SECITEM_ZfreeItem = nsslib.declare("SECITEM_ZfreeItem",
                                                    ctypes.default_abi, ctypes.void_t,
                                                    this.nss_t.SECItem.ptr, this.nss_t.PRBool);
        
        
        this.nss.SECITEM_FreeItem = nsslib.declare("SECITEM_FreeItem",
                                                   ctypes.default_abi, ctypes.void_t,
                                                   this.nss_t.SECItem.ptr, this.nss_t.PRBool);
        
        
        this.nss.SECOID_DestroyAlgorithmID = nsslib.declare("SECOID_DestroyAlgorithmID",
                                                            ctypes.default_abi, ctypes.void_t,
                                                            this.nss_t.SECAlgorithmID.ptr, this.nss_t.PRBool);
    },


    
    
    

    encrypt : function(clearTextUCS2, symmetricKey, iv) {
        this.log("encrypt() called");

        
        
        let inputBuffer = new ctypes.ArrayType(ctypes.unsigned_char)(clearTextUCS2);
        inputBuffer = ctypes.cast(inputBuffer, ctypes.unsigned_char.array(inputBuffer.length - 1));

        
        
        
        let outputBufferSize = inputBuffer.length + this.blockSize;
        let outputBuffer = new ctypes.ArrayType(ctypes.unsigned_char, outputBufferSize)();

        outputBuffer = this._commonCrypt(inputBuffer, outputBuffer, symmetricKey, iv, this.nss.CKA_ENCRYPT);

        return this.encodeBase64(outputBuffer.address(), outputBuffer.length);
    },


    decrypt : function(cipherText, symmetricKey, iv) {
        this.log("decrypt() called");

        let inputUCS2 = "";
        if (cipherText.length)
            inputUCS2 = atob(cipherText);

        
        
        
        
        
        let len   = inputUCS2.length;
        let input = new ctypes.ArrayType(ctypes.unsigned_char, len)();
        let ints  = ctypes.cast(input, ctypes.uint8_t.array(len));
        this.byteCompressInts(inputUCS2, ints, len);

        let outputBuffer = new ctypes.ArrayType(ctypes.unsigned_char, input.length)();

        outputBuffer = this._commonCrypt(input, outputBuffer, symmetricKey, iv, this.nss.CKA_DECRYPT);

        
        
        
        return "" + outputBuffer.readString() + "";
    },
        
    _commonCrypt : function (input, output, symmetricKey, iv, operation) {
        this.log("_commonCrypt() called");
        iv = atob(iv);

        
        
        if (iv.length > this.blockSize)
            iv = iv.slice(0, this.blockSize);

        
        this.byteCompressInts(iv, this._ivSECItemContents, iv.length);

        let ctx, symKey, ivParam;
        try {
            ivParam = this.nss.PK11_ParamFromIV(this.padMechanism, this._ivSECItem);
            if (ivParam.isNull())
                throw Components.Exception("can't convert IV to param", Cr.NS_ERROR_FAILURE);

            symKey = this.importSymKey(symmetricKey, operation);
            ctx = this.nss.PK11_CreateContextBySymKey(this.padMechanism, operation, symKey, ivParam);
            if (ctx.isNull())
                throw Components.Exception("couldn't create context for symkey", Cr.NS_ERROR_FAILURE);

            let maxOutputSize = output.length;
            if (this.nss.PK11_CipherOp(ctx, output, this._commonCryptSignedOutputSizeAddr, maxOutputSize, input, input.length))
                throw Components.Exception("cipher operation failed", Cr.NS_ERROR_FAILURE);

            let actualOutputSize = this._commonCryptSignedOutputSize.value;
            let finalOutput = output.addressOfElement(actualOutputSize);
            maxOutputSize -= actualOutputSize;

            
            
            
            if (this.nss.PK11_DigestFinal(ctx, finalOutput, this._commonCryptUnsignedOutputSizeAddr, maxOutputSize))
                throw Components.Exception("cipher finalize failed", Cr.NS_ERROR_FAILURE);

            actualOutputSize += this._commonCryptUnsignedOutputSize.value;
            let newOutput = ctypes.cast(output, ctypes.unsigned_char.array(actualOutputSize));
            return newOutput;
        } catch (e) {
            this.log("_commonCrypt: failed: " + e);
            throw e;
        } finally {
            if (ctx && !ctx.isNull())
                this.nss.PK11_DestroyContext(ctx, true);
            if (ivParam && !ivParam.isNull())
                this.nss.SECITEM_FreeItem(ivParam, true);

            
            
        }
    },


    generateRandomKey : function() {
        this.log("generateRandomKey() called");
        let slot, randKey, keydata;
        try {
            slot = this.nss.PK11_GetInternalSlot();
            if (slot.isNull())
                throw Components.Exception("couldn't get internal slot", Cr.NS_ERROR_FAILURE);

            randKey = this.nss.PK11_KeyGen(slot, this.keygenMechanism, null, this.keySize, null);
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

    generateRandomIV : function() this.generateRandomBytes(this.ivLength),

    generateRandomBytes : function(byteCount) {
        this.log("generateRandomBytes() called");

        
        let scratch = new ctypes.ArrayType(ctypes.unsigned_char, byteCount)();
        if (this.nss.PK11_GenerateRandom(scratch, byteCount))
            throw Components.Exception("PK11_GenrateRandom failed", Cr.NS_ERROR_FAILURE);

        return this.encodeBase64(scratch.address(), scratch.length);
    },

    
    
    

    
    
    
    _encryptionSymKeyMemo: {},
    _decryptionSymKeyMemo: {},
    importSymKey: function importSymKey(encodedKeyString, operation) {
        let memo;

        
        
        switch (operation) {
            case this.nss.CKA_ENCRYPT:
                memo = this._encryptionSymKeyMemo;
                break;
            case this.nss.CKA_DECRYPT:
                memo = this._decryptionSymKeyMemo;
                break;
            default:
                throw "Unsupported operation in importSymKey.";
        }

        if (encodedKeyString in memo)
            return memo[encodedKeyString];

        let keyItem, slot;
        try {
            keyItem = this.makeSECItem(encodedKeyString, true);
            slot    = this.nss.PK11_GetInternalKeySlot();
            if (slot.isNull())
                throw Components.Exception("can't get internal key slot",
                                           Cr.NS_ERROR_FAILURE);

            let symKey = this.nss.PK11_ImportSymKey(slot, this.padMechanism,
                                                    this.nss.PK11_OriginUnwrap,
                                                    operation, keyItem, null);
            if (!symKey || symKey.isNull())
                throw Components.Exception("symkey import failed",
                                           Cr.NS_ERROR_FAILURE);

            return memo[encodedKeyString] = symKey;
        } finally {
            if (slot && !slot.isNull())
                this.nss.PK11_FreeSlot(slot);
            this.freeSECItem(keyItem);
        }
    },


    
    
    

    





    byteCompressInts : function byteCompressInts (jsString, intArray, count) {
        let len = jsString.length;
        let end = Math.min(len, count);

        for (let i = 0; i < end; i++)
            intArray[i] = jsString.charCodeAt(i) % 256; 

        
        for (let i = len; i < count; i++)
            intArray[i] = 0;
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

        let len = input.length;
        let item = this.nss.SECITEM_AllocItem(null, null, len);
        if (item.isNull())
            throw "SECITEM_AllocItem failed.";

        let ptr  = ctypes.cast(item.contents.data,
                               ctypes.unsigned_char.array(len).ptr);
        let dest = ctypes.cast(ptr.contents, ctypes.uint8_t.array(len));
        this.byteCompressInts(input, dest, len);
        return item;
    },

    freeSECItem : function(zap) {
        if (zap && !zap.isNull())
            this.nss.SECITEM_ZfreeItem(zap, true);
    },

    
    
    
    _ivSECItem: null,
    _ivSECItemContents: null,

    initIVSECItem: function initIVSECItem() {
        if (this._ivSECItem) {
            this._ivSECItemContents = null;
            this.freeSECItem(this._ivSECItem);
        }

        let item = this.nss.SECITEM_AllocItem(null, null, this.blockSize);
        if (item.isNull())
            throw "SECITEM_AllocItem failed.";

        let ptr = ctypes.cast(item.contents.data,
                              ctypes.unsigned_char.array(this.blockSize).ptr);
        let contents = ctypes.cast(ptr.contents,
                                   ctypes.uint8_t.array(this.blockSize));
        this._ivSECItem = item;
        this._ivSECItemContents = contents;
    },

    


    deriveKeyFromPassphrase : function deriveKeyFromPassphrase(passphrase, salt, keyLength) {
        this.log("deriveKeyFromPassphrase() called.");
        let passItem = this.makeSECItem(passphrase, false);
        let saltItem = this.makeSECItem(salt, true);

        let pbeAlg    = ALGORITHM;
        let cipherAlg = ALGORITHM;   

        
        let prfAlg    = this.nss.SEC_OID_HMAC_SHA1;

        let keyLength  = keyLength || 0;    
        let iterations = KEY_DERIVATION_ITERATIONS;

        let algid, slot, symKey, keyData;
        try {
            algid = this.nss.PK11_CreatePBEV2AlgorithmID(pbeAlg, cipherAlg, prfAlg,
                                                         keyLength, iterations,
                                                         saltItem);
            if (algid.isNull())
                throw Components.Exception("PK11_CreatePBEV2AlgorithmID failed", Cr.NS_ERROR_FAILURE);

            slot = this.nss.PK11_GetInternalSlot();
            if (slot.isNull())
                throw Components.Exception("couldn't get internal slot", Cr.NS_ERROR_FAILURE);

            symKey = this.nss.PK11_PBEKeyGen(slot, algid, passItem, false, null);
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

            this.freeSECItem(passItem);
            this.freeSECItem(saltItem);
        }
    },
};
