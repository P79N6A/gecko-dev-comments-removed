



const { classes: Cc, interfaces: Ci, utils: Cu, results: Cr } = Components;

Components.utils.import("resource://gre/modules/XPCOMUtils.jsm");
Components.utils.import("resource://gre/modules/Services.jsm");

XPCOMUtils.defineLazyModuleGetter(this, "LoginHelper",
                                  "resource://gre/modules/LoginHelper.jsm");

function LoginManagerCrypto_SDR() {
  this.init();
};

LoginManagerCrypto_SDR.prototype = {

  classID : Components.ID("{dc6c2976-0f73-4f1f-b9ff-3d72b4e28309}"),
  QueryInterface : XPCOMUtils.generateQI([Ci.nsILoginManagerCrypto]),

  __sdrSlot : null, 
  get _sdrSlot() {
    if (!this.__sdrSlot) {
      let modules = Cc["@mozilla.org/security/pkcs11moduledb;1"].
                    getService(Ci.nsIPKCS11ModuleDB);
      this.__sdrSlot = modules.findSlotByName("");
    }
    return this.__sdrSlot;
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

  _uiBusy : false,


  init : function () {
    
    
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

    let wasLoggedIn = this.isLoggedIn;
    let canceledMP = false;

    this._uiBusy = true;
    try {
      let plainOctet = this._utfConverter.ConvertFromUnicode(plainText);
      plainOctet += this._utfConverter.Finish();
      cipherText = this._decoderRing.encryptString(plainOctet);
    } catch (e) {
      this.log("Failed to encrypt string. (" + e.name + ")");
      
      
      if (e.result == Cr.NS_ERROR_FAILURE) {
        canceledMP = true;
        throw Components.Exception("User canceled master password entry", Cr.NS_ERROR_ABORT);
      } else {
        throw Components.Exception("Couldn't encrypt string", Cr.NS_ERROR_FAILURE);
      }
    } finally {
      this._uiBusy = false;
      
      if (!wasLoggedIn && this.isLoggedIn)
        this._notifyObservers("passwordmgr-crypto-login");
      else if (canceledMP)
        this._notifyObservers("passwordmgr-crypto-loginCanceled");
    }
    return cipherText;
  },


  







  decrypt : function (cipherText) {
    let plainText = null;

    let wasLoggedIn = this.isLoggedIn;
    let canceledMP = false;

    this._uiBusy = true;
    try {
      let plainOctet;
      plainOctet = this._decoderRing.decryptString(cipherText);
      plainText = this._utfConverter.ConvertToUnicode(plainOctet);
    } catch (e) {
      this.log("Failed to decrypt string: " + cipherText +
          " (" + e.name + ")");

      
      this._utfConverterReset();

      
      
      
      
      if (e.result == Cr.NS_ERROR_NOT_AVAILABLE) {
        canceledMP = true;
        throw Components.Exception("User canceled master password entry", Cr.NS_ERROR_ABORT);
      } else {
        throw Components.Exception("Couldn't decrypt string", Cr.NS_ERROR_FAILURE);
      }
    } finally {
      this._uiBusy = false;
      
      if (!wasLoggedIn && this.isLoggedIn)
        this._notifyObservers("passwordmgr-crypto-login");
      else if (canceledMP)
        this._notifyObservers("passwordmgr-crypto-loginCanceled");
    }

    return plainText;
  },


  


  get uiBusy() {
    return this._uiBusy;
  },


  


  get isLoggedIn() {
    let status = this._sdrSlot.status;
    this.log("SDR slot status is " + status);
    if (status == Ci.nsIPKCS11Slot.SLOT_READY ||
        status == Ci.nsIPKCS11Slot.SLOT_LOGGED_IN)
      return true;
    if (status == Ci.nsIPKCS11Slot.SLOT_NOT_LOGGED_IN)
      return false;
    throw Components.Exception("unexpected slot status: " + status, Cr.NS_ERROR_FAILURE);
  },


  


  get defaultEncType() {
    return Ci.nsILoginManagerCrypto.ENCTYPE_SDR;
  },


  


  _notifyObservers : function(topic) {
    this.log("Prompted for a master password, notifying for " + topic);
    Services.obs.notifyObservers(null, topic, null);
  },
}; 

XPCOMUtils.defineLazyGetter(this.LoginManagerCrypto_SDR.prototype, "log", () => {
  let logger = LoginHelper.createLogger("Login crypto");
  return logger.log.bind(logger);
});

let component = [LoginManagerCrypto_SDR];
this.NSGetFactory = XPCOMUtils.generateNSGetFactory(component);
