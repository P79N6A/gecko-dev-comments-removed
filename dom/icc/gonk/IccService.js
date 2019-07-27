



"use strict";

const {classes: Cc, interfaces: Ci, utils: Cu, results: Cr} = Components;

Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://gre/modules/Services.jsm");

var RIL = {};
Cu.import("resource://gre/modules/ril_consts.js", RIL);

const GONK_ICCSERVICE_CONTRACTID = "@mozilla.org/icc/gonkiccservice;1";
const GONK_ICCSERVICE_CID = Components.ID("{df854256-9554-11e4-a16c-c39e8d106c26}");

const NS_XPCOM_SHUTDOWN_OBSERVER_ID      = "xpcom-shutdown";
const NS_PREFBRANCH_PREFCHANGE_TOPIC_ID  = "nsPref:changed";

const kPrefRilDebuggingEnabled = "ril.debugging.enabled";
const kPrefRilNumRadioInterfaces = "ril.numRadioInterfaces";

XPCOMUtils.defineLazyServiceGetter(this, "gRadioInterfaceLayer",
                                   "@mozilla.org/ril;1",
                                   "nsIRadioInterfaceLayer");

let DEBUG = RIL.DEBUG_RIL;
function debug(s) {
  dump("IccService: " + s);
}

function IccInfo() {}
IccInfo.prototype = {
  QueryInterface: XPCOMUtils.generateQI([Ci.nsIIccInfo]),

  

  iccType: null,
  iccid: null,
  mcc: null,
  mnc: null,
  spn: null,
  isDisplayNetworkNameRequired: false,
  isDisplaySpnRequired: false
};

function GsmIccInfo() {}
GsmIccInfo.prototype = {
  __proto__: IccInfo.prototype,
  QueryInterface: XPCOMUtils.generateQI([Ci.nsIGsmIccInfo,
                                         Ci.nsIIccInfo]),

  

  msisdn: null
};

function CdmaIccInfo() {}
CdmaIccInfo.prototype = {
  __proto__: IccInfo.prototype,
  QueryInterface: XPCOMUtils.generateQI([Ci.nsICdmaIccInfo,
                                         Ci.nsIIccInfo]),

  

  mdn: null,
  prlVersion: 0
};

function IccService() {
  this._iccs = [];

  let numClients = gRadioInterfaceLayer.numRadioInterfaces;
  for (let i = 0; i < numClients; i++) {
    this._iccs.push(new Icc(gRadioInterfaceLayer.getRadioInterface(i)));
  }

  this._updateDebugFlag();

  Services.prefs.addObserver(kPrefRilDebuggingEnabled, this, false);
  Services.obs.addObserver(this, NS_XPCOM_SHUTDOWN_OBSERVER_ID, false);
}
IccService.prototype = {
  classID: GONK_ICCSERVICE_CID,

  classInfo: XPCOMUtils.generateCI({classID: GONK_ICCSERVICE_CID,
                                    contractID: GONK_ICCSERVICE_CONTRACTID,
                                    classDescription: "IccService",
                                    interfaces: [Ci.nsIIccService,
                                                 Ci.nsIGonkIccService],
                                    flags: Ci.nsIClassInfo.SINGLETON}),

  QueryInterface: XPCOMUtils.generateQI([Ci.nsIIccService,
                                         Ci.nsIGonkIccService,
                                         Ci.nsIObserver]),

  
  _iccs: null,

  _updateDebugFlag: function() {
    try {
      DEBUG = DEBUG ||
              Services.prefs.getBoolPref(kPrefRilDebuggingEnabled);
    } catch (e) {}
  },

  


  getIccByServiceId: function(aServiceId) {
    let icc = this._iccs[aServiceId];
    if (!icc) {
      throw Cr.NS_ERROR_UNEXPECTED;
    }

    return icc;
  },

  


  notifyCardStateChanged: function(aServiceId, aCardState) {
    if (DEBUG) {
      debug("notifyCardStateChanged for service Id: " + aServiceId +
            ", CardState: " + aCardState);
    }

    this.getIccByServiceId(aServiceId)._updateCardState(aCardState);
  },

  notifyIccInfoChanged: function(aServiceId, aIccInfo) {
    if (DEBUG) {
      debug("notifyIccInfoChanged for service Id: " + aServiceId +
            ", IccInfo: " + JSON.stringify(aIccInfo));
    }

    this.getIccByServiceId(aServiceId)._updateIccInfo(aIccInfo);
  },

  notifyImsiChanged: function(aServiceId, aImsi) {
    if (DEBUG) {
      debug("notifyImsiChanged for service Id: " + aServiceId +
            ", Imsi: " + aImsi);
    }

    let icc = this.getIccByServiceId(aServiceId);
    icc._imsi = aImsi;
  },

  


  observe: function(aSubject, aTopic, aData) {
    switch (aTopic) {
      case NS_PREFBRANCH_PREFCHANGE_TOPIC_ID:
        if (aData === kPrefRilDebuggingEnabled) {
          this._updateDebugFlag();
        }
        break;
      case NS_XPCOM_SHUTDOWN_OBSERVER_ID:
        Services.prefs.removeObserver(kPrefRilDebuggingEnabled, this);
        Services.obs.removeObserver(this, NS_XPCOM_SHUTDOWN_OBSERVER_ID);
        break;
    }
  }
};

function Icc(aRadioInterface) {
  this._radioInterface = aRadioInterface;
  this._listeners = [];
}
Icc.prototype = {
  QueryInterface: XPCOMUtils.generateQI([Ci.nsIIcc]),

  _radioInterface: null,
  _imsi: null,
  _listeners: null,

  _updateCardState: function(aCardState) {
    if (this.cardState != aCardState) {
      this.cardState = aCardState;
    }

    this._deliverListenerEvent("notifyCardStateChanged");
  },

  
  _updateInfo: function(aSrcInfo, aDestInfo) {
    for (let key in aSrcInfo) {
      aDestInfo[key] = aSrcInfo[key];
    }
  },

  




  _updateIccInfo: function(aIccInfo) {
    
    if (!aIccInfo || !aIccInfo.iccid) {
      if (this.iccInfo) {
        if (DEBUG) {
          debug("Card is not detected, clear iccInfo to null.");
        }
        this.iccInfo = null;
        this._deliverListenerEvent("notifyIccInfoChanged");
      }
      return;
    }

    
    if (!this.iccInfo ||
        this.iccInfo.iccType != aIccInfo.iccType) {
      if (aIccInfo.iccType === "ruim" || aIccInfo.iccType === "csim") {
        this.iccInfo = new CdmaIccInfo();
      } else if (aIccInfo.iccType === "sim" || aIccInfo.iccType === "usim") {
        this.iccInfo = new GsmIccInfo();
      } else {
        this.iccInfo = new IccInfo();
      }
    }

    this._updateInfo(aIccInfo, this.iccInfo);

    this._deliverListenerEvent("notifyIccInfoChanged");

    
    if (aIccInfo.mcc) {
      try {
        Services.prefs.setCharPref("ril.lastKnownSimMcc",
                                   aIccInfo.mcc.toString());
      } catch (e) {}
    }
  },

  _deliverListenerEvent: function(aName, aArgs) {
    let listeners = this._listeners.slice();
    for (let listener of listeners) {
      if (this._listeners.indexOf(listener) === -1) {
        continue;
      }
      let handler = listener[aName];
      if (typeof handler != "function") {
        throw new Error("No handler for " + aName);
      }
      try {
        handler.apply(listener, aArgs);
      } catch (e) {
        if (DEBUG) {
          debug("listener for " + aName + " threw an exception: " + e);
        }
      }
    }
  },

  _modifyCardLock: function(aOperation, aOptions, aCallback) {
    this._radioInterface.sendWorkerMessage(aOperation,
                                           aOptions,
                                           (aResponse) => {
      if (aResponse.errorMsg) {
        let retryCount =
          (aResponse.retryCount !== undefined) ? aResponse.retryCount : -1;
        aCallback.notifyCardLockError(aResponse.errorMsg, retryCount);
        return;
      }

      aCallback.notifySuccess();
    });
  },

  














  _isImsiMatches: function(aMvnoData, aImsi) {
    
    if (aMvnoData.length > aImsi.length) {
      return false;
    }

    for (let i = 0; i < aMvnoData.length; i++) {
      let c = aMvnoData[i];
      if ((c !== 'x') && (c !== 'X') && (c !== aImsi[i])) {
        return false;
      }
    }
    return true;
  },

  


  iccInfo: null,
  cardState: Ci.nsIIcc.CARD_STATE_UNKNOWN,

  registerListener: function(aListener) {
    if (this._listeners.indexOf(aListener) >= 0) {
      throw Cr.NS_ERROR_UNEXPECTED;
    }

    this._listeners.push(aListener);
  },

  unregisterListener: function(aListener) {
    let index = this._listeners.indexOf(aListener);
    if (index >= 0) {
      this._listeners.splice(index, 1);
    }
  },

  getCardLockEnabled: function(aLockType, aCallback) {
    this._radioInterface.sendWorkerMessage("iccGetCardLockEnabled",
                                           { lockType: aLockType },
                                           (aResponse) => {
      if (aResponse.errorMsg) {
        aCallback.notifyError(aResponse.errorMsg);
        return;
      }

      aCallback.notifySuccessWithBoolean(aResponse.enabled);
    });
  },

  unlockCardLock: function(aLockType, aPassword, aNewPin, aCallback) {
    this._modifyCardLock("iccUnlockCardLock",
                         { lockType: aLockType,
                           password: aPassword,
                           newPin: aNewPin },
                         aCallback);
  },

  setCardLockEnabled: function(aLockType, aPassword, aEnabled, aCallback) {
    this._modifyCardLock("iccSetCardLockEnabled",
                         { lockType: aLockType,
                           password: aPassword,
                           enabled: aEnabled },
                         aCallback);
  },

  changeCardLockPassword: function(aLockType, aPassword, aNewPassword, aCallback) {
    this._modifyCardLock("iccChangeCardLockPassword",
                         { lockType: aLockType,
                           password: aPassword,
                           newPassword: aNewPassword, },
                         aCallback);
  },

  getCardLockRetryCount: function(aLockType, aCallback) {
    this._radioInterface.sendWorkerMessage("iccGetCardLockRetryCount",
                                           { lockType: aLockType },
                                           (aResponse) => {
      if (aResponse.errorMsg) {
        aCallback.notifyError(aResponse.errorMsg);
        return;
      }

      aCallback.notifyGetCardLockRetryCount(aResponse.retryCount);
    });
  },

  matchMvno: function(aMvnoType, aMvnoData, aCallback) {
    if (!aMvnoData) {
      aCallback.notifyError(RIL.GECKO_ERROR_INVALID_PARAMETER);
      return;
    }

    switch (aMvnoType) {
      case Ci.nsIIcc.CARD_MVNO_TYPE_IMSI:
        let imsi = this._imsi;
        if (!imsi) {
          aCallback.notifyError(RIL.GECKO_ERROR_GENERIC_FAILURE);
          break;
        }
        aCallback.notifySuccessWithBoolean(
          this._isImsiMatches(aMvnoData, imsi));
        break;
      case Ci.nsIIcc.CARD_MVNO_TYPE_SPN:
        let spn = this.iccInfo && this.iccInfo.spn;
        if (!spn) {
          aCallback.notifyError(RIL.GECKO_ERROR_GENERIC_FAILURE);
          break;
        }
        aCallback.notifySuccessWithBoolean(spn == aMvnoData);
        break;
      case Ci.nsIIcc.CARD_MVNO_TYPE_GID:
        this._radioInterface.sendWorkerMessage("getGID1",
                                               null,
                                               (aResponse) => {
          let gid = aResponse.gid1;
          let mvnoDataLength = aMvnoData.length;

          if (!gid) {
            aCallback.notifyError(RIL.GECKO_ERROR_GENERIC_FAILURE);
          } else if (mvnoDataLength > gid.length) {
            aCallback.notifySuccessWithBoolean(false);
          } else {
            let result =
              gid.substring(0, mvnoDataLength).toLowerCase() ==
              aMvnoData.toLowerCase();
            aCallback.notifySuccessWithBoolean(result);
          }
        });
        break;
      default:
        aCallback.notifyError(RIL.GECKO_ERROR_MODE_NOT_SUPPORTED);
        break;
    }
  },

  getServiceStateEnabled: function(aService, aCallback) {
    this._radioInterface.sendWorkerMessage("getIccServiceState",
                                           { service: aService },
                                           (aResponse) => {
      if (aResponse.errorMsg) {
        aCallback.notifyError(aResponse.errorMsg);
        return;
      }

      aCallback.notifySuccessWithBoolean(aResponse.result);
    });
  }
};

this.NSGetFactory = XPCOMUtils.generateNSGetFactory([IccService]);
