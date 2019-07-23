




































const Cc = Components.classes;
const Ci = Components.interfaces;
const Cr = Components.results;

Components.utils.import("resource://gre/modules/XPCOMUtils.jsm");

function LoginManagerCrypto_SDR() {
    this.init();
};

LoginManagerCrypto_SDR.prototype = {

    classDescription  : "LoginManagerCrypto_SDR",
    contractID : "@mozilla.org/login-manager/crypto/SDR;1",
    classID : Components.ID("{dc6c2976-0f73-4f1f-b9ff-3d72b4e28309}"),
    QueryInterface : XPCOMUtils.generateQI([Ci.nsILoginManagerCrypto]),

    __logService : null, 
    get _logService() {
        if (!this.__logService)
            this.__logService = Cc["@mozilla.org/consoleservice;1"].
                                getService(Ci.nsIConsoleService);
        return this.__logService;
    },

    __decoderRing : null,  
    get _decoderRing() {
        if (!this.__decoderRing)
            this.__decoderRing = Cc["@mozilla.org/security/sdr;1"].
                                 getService(Ci.nsISecretDecoderRing);
        return this.__decoderRing;
    },

    __utfConverter : null, 
    get _utfConverter() {
        if (!this.__utfConverter) {
            this.__utfConverter = Cc["@mozilla.org/intl/scriptableunicodeconverter"].
                                  createInstance(Ci.nsIScriptableUnicodeConverter);
            this.__utfConverter.charset = "UTF-8";
        }
        return this.__utfConverter;
    },

    _utfConverterReset : function() {
        this.__utfConverter = null;
    },

    __observerService : null,
    get _observerService() {
        if (!this.__observerService)
            this.__observerService = Cc["@mozilla.org/observer-service;1"].
                                     getService(Ci.nsIObserverService);
        return this.__observerService;
    },

    _debug : false, 


    




    log : function (message) {
        if (!this._debug)
            return;
        dump("PwMgr cryptoSDR: " + message + "\n");
        this._logService.logStringMessage("PwMgr cryptoSDR: " + message);
    },


    init : function () {
        
        this._prefBranch = Cc["@mozilla.org/preferences-service;1"].
                           getService(Ci.nsIPrefService);
        this._prefBranch = this._prefBranch.getBranch("signon.");
        this._prefBranch.QueryInterface(Ci.nsIPrefBranch2);

        this._debug = this._prefBranch.getBoolPref("debug");

        
        
        let tokenDB = Cc["@mozilla.org/security/pk11tokendb;1"].
                      getService(Ci.nsIPK11TokenDB);

        let token = tokenDB.getInternalKeyToken();
        if (token.needsUserInit) {
            this.log("Initializing key3.db with default blank password.");
            token.initPassword("");
        }
    },


    







    encrypt : function (plainText) {
        let cipherText = null;

        try {
            let plainOctet = this._utfConverter.ConvertFromUnicode(plainText);
            plainOctet += this._utfConverter.Finish();
            cipherText = this._decoderRing.encryptString(plainOctet);
        } catch (e) {
            this.log("Failed to encrypt string. (" + e.name + ")");
            
            
            if (e.result == Cr.NS_ERROR_FAILURE)
                throw Components.Exception("User canceled master password entry", Cr.NS_ERROR_ABORT);
            else
                throw Components.Exception("Couldn't encrypt string", Cr.NS_ERROR_FAILURE);
        }
        return cipherText;
    },


    







    decrypt : function (cipherText) {
        let plainText = null;

        try {
            let plainOctet;
            if (cipherText.charAt(0) == '~') {
                
                
                
                plainOctet = atob(cipherText.substring(1));
            } else {
                plainOctet = this._decoderRing.decryptString(cipherText);
            }
            plainText = this._utfConverter.ConvertToUnicode(plainOctet);
        } catch (e) {
            this.log("Failed to decrypt string: " + cipherText +
                " (" + e.name + ")");

            
            this._utfConverterReset();

            
            
            
            
            if (e.result == Cr.NS_ERROR_NOT_AVAILABLE)
                throw Components.Exception("User canceled master password entry", Cr.NS_ERROR_ABORT);
            else
                throw Components.Exception("Couldn't decrypt string", Cr.NS_ERROR_FAILURE);
        }

        return plainText;
    }
}; 

let component = [LoginManagerCrypto_SDR];
function NSGetModule(compMgr, fileSpec) {
    return XPCOMUtils.generateModule(component);
}
