




"use strict";

const {classes: Cc, interfaces: Ci, utils: Cu, results: Cr} = Components;

Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://gre/modules/Services.jsm");
Cu.import("resource://gre/modules/Promise.jsm");
Cu.import("resource://gre/modules/systemlibs.js");

XPCOMUtils.defineLazyGetter(this, "RIL", function () {
  let obj = {};
  Cu.import("resource://gre/modules/ril_consts.js", obj);
  return obj;
});

const GONK_TELEPHONYSERVICE_CONTRACTID =
  "@mozilla.org/telephony/gonktelephonyservice;1";
const GONK_TELEPHONYSERVICE_CID =
  Components.ID("{67d26434-d063-4d28-9f48-5b3189788155}");
const MOBILECALLFORWARDINGOPTIONS_CID =
  Components.ID("{79b5988b-9436-48d8-a652-88fa033f146c}");

const NS_XPCOM_SHUTDOWN_OBSERVER_ID = "xpcom-shutdown";

const NS_PREFBRANCH_PREFCHANGE_TOPIC_ID = "nsPref:changed";

const kPrefRilNumRadioInterfaces = "ril.numRadioInterfaces";
const kPrefRilDebuggingEnabled = "ril.debugging.enabled";
const kPrefDefaultServiceId = "dom.telephony.defaultServiceId";

const nsITelephonyAudioService = Ci.nsITelephonyAudioService;
const nsITelephonyService = Ci.nsITelephonyService;

const CALL_WAKELOCK_TIMEOUT = 5000;


const CDMA_SECOND_CALL_INDEX = 2;

const DIAL_ERROR_INVALID_STATE_ERROR = "InvalidStateError";
const DIAL_ERROR_OTHER_CONNECTION_IN_USE = "OtherConnectionInUse";
const DIAL_ERROR_BAD_NUMBER = RIL.GECKO_CALL_ERROR_BAD_NUMBER;

const DEFAULT_EMERGENCY_NUMBERS = ["112", "911"];


const MMI_MATCH_GROUP_FULL_MMI = 1;
const MMI_MATCH_GROUP_PROCEDURE = 2;
const MMI_MATCH_GROUP_SERVICE_CODE = 3;
const MMI_MATCH_GROUP_SIA = 4;
const MMI_MATCH_GROUP_SIB = 5;
const MMI_MATCH_GROUP_SIC = 6;
const MMI_MATCH_GROUP_PWD_CONFIRM = 7;
const MMI_MATCH_GROUP_DIALING_NUMBER = 8;

let DEBUG;
function debug(s) {
  dump("TelephonyService: " + s + "\n");
}

XPCOMUtils.defineLazyServiceGetter(this, "gRadioInterfaceLayer",
                                   "@mozilla.org/ril;1",
                                   "nsIRadioInterfaceLayer");

XPCOMUtils.defineLazyServiceGetter(this, "gPowerManagerService",
                                   "@mozilla.org/power/powermanagerservice;1",
                                   "nsIPowerManagerService");

XPCOMUtils.defineLazyServiceGetter(this, "gSystemMessenger",
                                   "@mozilla.org/system-message-internal;1",
                                   "nsISystemMessagesInternal");

XPCOMUtils.defineLazyServiceGetter(this, "gAudioService",
                                   "@mozilla.org/telephony/audio-service;1",
                                   "nsITelephonyAudioService");

XPCOMUtils.defineLazyServiceGetter(this, "gGonkMobileConnectionService",
                                   "@mozilla.org/mobileconnection/mobileconnectionservice;1",
                                   "nsIGonkMobileConnectionService");

XPCOMUtils.defineLazyGetter(this, "gPhoneNumberUtils", function() {
  let ns = {};
  Cu.import("resource://gre/modules/PhoneNumberUtils.jsm", ns);
  return ns.PhoneNumberUtils;
});

function MobileCallForwardingOptions(aOptions) {
  for (let key in aOptions) {
    this[key] = aOptions[key];
  }
}
MobileCallForwardingOptions.prototype = {
  QueryInterface: XPCOMUtils.generateQI([Ci.nsIMobileCallForwardingOptions]),
  classID: MOBILECALLFORWARDINGOPTIONS_CID,
  classInfo: XPCOMUtils.generateCI({
    classID:          MOBILECALLFORWARDINGOPTIONS_CID,
    classDescription: "MobileCallForwardingOptions",
    interfaces:       [Ci.nsIMobileCallForwardingOptions]
  }),

  

  active: false,
  action: Ci.nsIMobileConnection.CALL_FORWARD_ACTION_UNKNOWN,
  reason: Ci.nsIMobileConnection.CALL_FORWARD_REASON_UNKNOWN,
  number: null,
  timeSeconds: -1,
  serviceClass: Ci.nsIMobileConnection.ICC_SERVICE_CLASS_NONE
};

function TelephonyService() {
  this._numClients = gRadioInterfaceLayer.numRadioInterfaces;
  this._listeners = [];

  this._mmiRegExp = null;

  this._isDialing = false;
  this._cachedDialRequest = null;
  this._currentCalls = {};
  this._audioStates = {};

  this._cdmaCallWaitingNumber = null;

  
  this._isActiveCall = {};
  this._numActiveCall = 0;

  this._updateDebugFlag();
  this.defaultServiceId = this._getDefaultServiceId();

  Services.prefs.addObserver(kPrefRilDebuggingEnabled, this, false);
  Services.prefs.addObserver(kPrefDefaultServiceId, this, false);

  Services.obs.addObserver(this, NS_XPCOM_SHUTDOWN_OBSERVER_ID, false);

  for (let i = 0; i < this._numClients; ++i) {
    this._enumerateCallsForClient(i);
    this._isActiveCall[i] = {};
    this._audioStates[i] = RIL.AUDIO_STATE_NO_CALL;
  }
}
TelephonyService.prototype = {
  classID: GONK_TELEPHONYSERVICE_CID,
  classInfo: XPCOMUtils.generateCI({classID: GONK_TELEPHONYSERVICE_CID,
                                    contractID: GONK_TELEPHONYSERVICE_CONTRACTID,
                                    classDescription: "TelephonyService",
                                    interfaces: [Ci.nsITelephonyService,
                                                 Ci.nsIGonkTelephonyService],
                                    flags: Ci.nsIClassInfo.SINGLETON}),
  QueryInterface: XPCOMUtils.generateQI([Ci.nsITelephonyService,
                                         Ci.nsIGonkTelephonyService,
                                         Ci.nsIObserver]),

  
  
  
  _callRingWakeLock: null,
  _callRingWakeLockTimer: null,

  _acquireCallRingWakeLock: function() {
    if (!this._callRingWakeLock) {
      if (DEBUG) debug("Acquiring a CPU wake lock for handling incoming call.");
      this._callRingWakeLock = gPowerManagerService.newWakeLock("cpu");
    }
    if (!this._callRingWakeLockTimer) {
      if (DEBUG) debug("Creating a timer for releasing the CPU wake lock.");
      this._callRingWakeLockTimer =
        Cc["@mozilla.org/timer;1"].createInstance(Ci.nsITimer);
    }
    if (DEBUG) debug("Setting the timer for releasing the CPU wake lock.");
    this._callRingWakeLockTimer
        .initWithCallback(this._releaseCallRingWakeLock.bind(this),
                          CALL_WAKELOCK_TIMEOUT, Ci.nsITimer.TYPE_ONE_SHOT);
  },

  _releaseCallRingWakeLock: function() {
    if (DEBUG) debug("Releasing the CPU wake lock for handling incoming call.");
    if (this._callRingWakeLockTimer) {
      this._callRingWakeLockTimer.cancel();
    }
    if (this._callRingWakeLock) {
      this._callRingWakeLock.unlock();
      this._callRingWakeLock = null;
    }
  },

  _getClient: function(aClientId) {
    return gRadioInterfaceLayer.getRadioInterface(aClientId);
  },

  _sendToRilWorker: function(aClientId, aType, aMessage, aCallback) {
    this._getClient(aClientId).sendWorkerMessage(aType, aMessage, aCallback);
  },

  
  _listeners: null,
  _notifyAllListeners: function(aMethodName, aArgs) {
    let listeners = this._listeners.slice();
    for (let listener of listeners) {
      if (this._listeners.indexOf(listener) == -1) {
        
        continue;
      }

      let handler = listener[aMethodName];
      try {
        handler.apply(listener, aArgs);
      } catch (e) {
        debug("listener for " + aMethodName + " threw an exception: " + e);
      }
    }
  },

  


  _updateActiveCall: function(aCall) {
    let active = false;
    let incoming = false;

    switch (aCall.state) {
      case nsITelephonyService.CALL_STATE_DIALING: 
      case nsITelephonyService.CALL_STATE_ALERTING:
      case nsITelephonyService.CALL_STATE_CONNECTED:
        active = true;
        break;
      case nsITelephonyService.CALL_STATE_INCOMING:
        incoming = true;
        break;
      case nsITelephonyService.CALL_STATE_HELD: 
      case nsITelephonyService.CALL_STATE_DISCONNECTED:
        break;
    }

    
    let oldActive = this._isActiveCall[aCall.clientId][aCall.callIndex];
    if (!oldActive && active) {
      this._numActiveCall++;
    } else if (oldActive && !active) {
      this._numActiveCall--;
    }
    this._isActiveCall[aCall.clientId][aCall.callIndex] = active;
  },

  _updateAudioState: function(aAudioState) {
    switch (aAudioState) {
      case RIL.AUDIO_STATE_NO_CALL:
        gAudioService.setPhoneState(nsITelephonyAudioService.PHONE_STATE_NORMAL);
        break;
      case RIL.AUDIO_STATE_INCOMING:
        gAudioService.setPhoneState(nsITelephonyAudioService.PHONE_STATE_RINGTONE);
        break;
      case RIL.AUDIO_STATE_IN_CALL:
        gAudioService.setPhoneState(nsITelephonyAudioService.PHONE_STATE_IN_CALL);
        break;
    }
  },

  _convertRILCallState: function(aState) {
    switch (aState) {
      case RIL.CALL_STATE_UNKNOWN:
        return nsITelephonyService.CALL_STATE_UNKNOWN;
      case RIL.CALL_STATE_ACTIVE:
        return nsITelephonyService.CALL_STATE_CONNECTED;
      case RIL.CALL_STATE_HOLDING:
        return nsITelephonyService.CALL_STATE_HELD;
      case RIL.CALL_STATE_DIALING:
        return nsITelephonyService.CALL_STATE_DIALING;
      case RIL.CALL_STATE_ALERTING:
        return nsITelephonyService.CALL_STATE_ALERTING;
      case RIL.CALL_STATE_INCOMING:
      case RIL.CALL_STATE_WAITING:
        return nsITelephonyService.CALL_STATE_INCOMING;
      default:
        throw new Error("Unknown rilCallState: " + aState);
    }
  },

  _convertRILSuppSvcNotification: function(aNotification) {
    switch (aNotification) {
      case RIL.GECKO_SUPP_SVC_NOTIFICATION_REMOTE_HELD:
        return nsITelephonyService.NOTIFICATION_REMOTE_HELD;
      case RIL.GECKO_SUPP_SVC_NOTIFICATION_REMOTE_RESUMED:
        return nsITelephonyService.NOTIFICATION_REMOTE_RESUMED;
      default:
        if (DEBUG) {
          debug("Unknown rilSuppSvcNotification: " + aNotification);
        }
        return;
    }
  },

  _rulesToCallForwardingOptions: function(aRules) {
    return aRules.map(rule => new MobileCallForwardingOptions(rule));
  },

  _updateDebugFlag: function() {
    try {
      DEBUG = RIL.DEBUG_RIL ||
              Services.prefs.getBoolPref(kPrefRilDebuggingEnabled);
    } catch (e) {}
  },

  _getDefaultServiceId: function() {
    let id = Services.prefs.getIntPref(kPrefDefaultServiceId);
    let numRil = Services.prefs.getIntPref(kPrefRilNumRadioInterfaces);

    if (id >= numRil || id < 0) {
      id = 0;
    }

    return id;
  },

  _currentCalls: null,
  _enumerateCallsForClient: function(aClientId) {
    if (DEBUG) debug("Enumeration of calls for client " + aClientId);

    this._sendToRilWorker(aClientId, "enumerateCalls", null, response => {
      if (!this._currentCalls[aClientId]) {
        this._currentCalls[aClientId] = {};
      }
      for (let call of response.calls) {
        call.clientId = aClientId;
        call.state = this._convertRILCallState(call.state);
        call.isSwitchable = true;
        call.isMergeable = true;

        this._currentCalls[aClientId][call.callIndex] = call;
      }
    });
  },

  






  _isEmergencyNumber: function(aNumber) {
    
    let numbers = libcutils.property_get("ril.ecclist") ||
                  libcutils.property_get("ro.ril.ecclist");
    if (numbers) {
      numbers = numbers.split(",");
    } else {
      
      numbers = DEFAULT_EMERGENCY_NUMBERS;
    }
    return numbers.indexOf(aNumber) != -1;
  },

  





  _isTemporaryCLIR: function(aMmi) {
    return (aMmi && aMmi.serviceCode === RIL.MMI_SC_CLIR) && aMmi.dialNumber;
  },

  





  _getTemporaryCLIRMode: function(aProcedure) {
    
    
    switch (aProcedure) {
      case RIL.MMI_PROCEDURE_ACTIVATION:
        return RIL.CLIR_SUPPRESSION;
      case RIL.MMI_PROCEDURE_DEACTIVATION:
        return RIL.CLIR_INVOCATION;
      default:
        return RIL.CLIR_DEFAULT;
    }
  },

  



  defaultServiceId: 0,

  registerListener: function(aListener) {
    if (this._listeners.indexOf(aListener) >= 0) {
      throw Cr.NS_ERROR_UNEXPECTED;
    }

    this._listeners.push(aListener);
  },

  unregisterListener: function(aListener) {
    let index = this._listeners.indexOf(aListener);
    if (index < 0) {
      throw Cr.NS_ERROR_UNEXPECTED;
    }

    this._listeners.splice(index, 1);
  },

  enumerateCalls: function(aListener) {
    if (DEBUG) debug("Requesting enumeration of calls for callback");

    for (let cid = 0; cid < this._numClients; ++cid) {
      let calls = this._currentCalls[cid];
      if (!calls) {
        continue;
      }
      for (let i = 0, indexes = Object.keys(calls); i < indexes.length; ++i) {
        let call = calls[indexes[i]];
        aListener.enumerateCallState(call.clientId, call.callIndex,
                                     call.state, call.number,
                                     call.numberPresentation, call.name,
                                     call.namePresentation, call.isOutgoing,
                                     call.isEmergency, call.isConference,
                                     call.isSwitchable, call.isMergeable);
      }
    }
    aListener.enumerateCallStateComplete();
  },

  _hasCalls: function(aClientId) {
    return Object.keys(this._currentCalls[aClientId]).length !== 0;
  },

  _hasCallsOnOtherClient: function(aClientId) {
    for (let cid = 0; cid < this._numClients; ++cid) {
      if (cid !== aClientId && this._hasCalls(cid)) {
        return true;
      }
    }
    return false;
  },

  
  _numCallsOnLine: function(aClientId) {
    let numCalls = 0;
    let hasConference = false;

    for (let cid in this._currentCalls[aClientId]) {
      let call = this._currentCalls[aClientId][cid];
      if (call.isConference) {
        hasConference = true;
      } else {
        numCalls++;
      }
    }

    return hasConference ? numCalls + 1 : numCalls;
  },

  


  _getOneActiveCall: function(aClientId) {
    for (let index in this._currentCalls[aClientId]) {
      let call = this._currentCalls[aClientId][index];
      if (call.state === nsITelephonyService.CALL_STATE_CONNECTED) {
        return call;
      }
    }
    return null;
  },

  _addCdmaChildCall: function(aClientId, aNumber, aParentId) {
    let childCall = {
      callIndex: CDMA_SECOND_CALL_INDEX,
      state: RIL.CALL_STATE_DIALING,
      number: aNumber,
      isOutgoing: true,
      isEmergency: false,
      isConference: false,
      isSwitchable: false,
      isMergeable: true,
      parentId: aParentId
    };

    
    this.notifyCallStateChanged(aClientId, childCall);

    childCall.state = RIL.CALL_STATE_ACTIVE;
    this.notifyCallStateChanged(aClientId, childCall);

    let parentCall = this._currentCalls[aClientId][childCall.parentId];
    parentCall.childId = CDMA_SECOND_CALL_INDEX;
    parentCall.state = RIL.CALL_STATE_HOLDING;
    parentCall.isSwitchable = false;
    parentCall.isMergeable = true;
    this.notifyCallStateChanged(aClientId, parentCall);
  },

  dial: function(aClientId, aNumber, aIsDialEmergency, aCallback) {
    if (DEBUG) debug("Dialing " + (aIsDialEmergency ? "emergency " : "") + aNumber);

    
    
    if (!aIsDialEmergency) {
      aNumber = gPhoneNumberUtils.normalize(aNumber);
    }

    
    
    if (!gPhoneNumberUtils.isPlainPhoneNumber(aNumber)) {
      if (DEBUG) debug("Error: Number '" + aNumber + "' is not viable. Drop.");
      aCallback.notifyError(DIAL_ERROR_BAD_NUMBER);
      return;
    }

    let mmi = this._parseMMI(aNumber, this._hasCalls(aClientId));
    if (!mmi) {
      this._dialCall(aClientId,
                     { number: aNumber,
                       isDialEmergency: aIsDialEmergency }, aCallback);
    } else if (this._isTemporaryCLIR(mmi)) {
      this._dialCall(aClientId,
                     { number: mmi.dialNumber,
                       clirMode: this._getTemporaryCLIRMode(mmi.procedure),
                       isDialEmergency: aIsDialEmergency }, aCallback);
    } else {
      
      if (aIsDialEmergency) {
        aCallback.notifyError(DIAL_ERROR_BAD_NUMBER);
        return;
      }

      this._dialMMI(aClientId, mmi, aCallback, true);
    }
  },

  




  _dialCall: function(aClientId, aOptions, aCallback) {
    if (this._isDialing) {
      if (DEBUG) debug("Error: Already has a dialing call.");
      aCallback.notifyError(DIAL_ERROR_INVALID_STATE_ERROR);
      return;
    }

    
    if (this._numCallsOnLine(aClientId) >= 2) {
      if (DEBUG) debug("Error: Already has more than 2 calls on line.");
      aCallback.notifyError(DIAL_ERROR_INVALID_STATE_ERROR);
      return;
    }

    
    
    if (this._hasCallsOnOtherClient(aClientId)) {
      if (DEBUG) debug("Error: Already has a call on other sim.");
      aCallback.notifyError(DIAL_ERROR_OTHER_CONNECTION_IN_USE);
      return;
    }

    aOptions.isEmergency = this._isEmergencyNumber(aOptions.number);
    if (aOptions.isEmergency) {
      
      aClientId = gRadioInterfaceLayer.getClientIdForEmergencyCall() ;
      if (aClientId === -1) {
        if (DEBUG) debug("Error: No client is avaialble for emergency call.");
        aCallback.notifyError(DIAL_ERROR_INVALID_STATE_ERROR);
        return;
      }
    }

    
    let activeCall = this._getOneActiveCall(aClientId);
    if (!activeCall) {
      this._sendDialCallRequest(aClientId, aOptions, aCallback);
    } else {
      if (DEBUG) debug("There is an active call. Hold it first before dial.");

      this._cachedDialRequest = {
        clientId: aClientId,
        options: aOptions,
        callback: aCallback
      };

      if (activeCall.isConference) {
        this.holdConference(aClientId);
      } else {
        this.holdCall(aClientId, activeCall.callIndex);
      }
    }
  },

  _sendDialCallRequest: function(aClientId, aOptions, aCallback) {
    this._isDialing = true;

    this._sendToRilWorker(aClientId, "dial", aOptions, response => {
      this._isDialing = false;

      if (!response.success) {
        aCallback.notifyError(response.errorMsg);
        return;
      }

      let currentCdmaCallIndex = !response.isCdma ? null :
        Object.keys(this._currentCalls[aClientId])[0];

      if (currentCdmaCallIndex == null) {
        aCallback.notifyDialCallSuccess(response.callIndex, response.number);
      } else {
        
        aCallback.notifyDialCallSuccess(CDMA_SECOND_CALL_INDEX, response.number);
        this._addCdmaChildCall(aClientId, response.number, currentCdmaCallIndex);
      }
    });
  },

  









  _dialMMI: function(aClientId, aMmi, aCallback, aStartNewSession) {
    let mmiServiceCode = aMmi ?
      this._serviceCodeToKeyString(aMmi.serviceCode) : RIL.MMI_KS_SC_USSD;

    aCallback.notifyDialMMI(mmiServiceCode);

    this._sendToRilWorker(aClientId, "sendMMI",
                          { mmi: aMmi,
                            startNewSession: aStartNewSession }, response => {
      if (DEBUG) debug("MMI response: " + JSON.stringify(response));

      if (!response.success) {
        if (response.additionalInformation != null) {
          aCallback.notifyDialMMIErrorWithInfo(response.errorMsg,
                                               response.additionalInformation);
        } else {
          aCallback.notifyDialMMIError(response.errorMsg);
        }
        return;
      }

      
      
      
      if (mmiServiceCode === RIL.MMI_KS_SC_IMEI &&
          !response.statusMessage) {
        aCallback.notifyDialMMIError(RIL.GECKO_ERROR_GENERIC_FAILURE);
        return;
      }

      
      
      
      if (mmiServiceCode === RIL.MMI_KS_SC_CALL_FORWARDING) {
        if (response.isSetCallForward) {
          gGonkMobileConnectionService.notifyCFStateChanged(aClientId,
                                                            response.action,
                                                            response.reason,
                                                            response.number,
                                                            response.timeSeconds,
                                                            response.serviceClass);
        }

        if (response.additionalInformation != null) {
          let callForwardingOptions =
            this._rulesToCallForwardingOptions(response.additionalInformation);
          aCallback.notifyDialMMISuccessWithCallForwardingOptions(
            response.statusMessage, callForwardingOptions.length, callForwardingOptions);
          return;
        }
      }

      
      if (response.additionalInformation == undefined) {
        aCallback.notifyDialMMISuccess(response.statusMessage);
        return;
      }

      
      if (!isNaN(parseInt(response.additionalInformation, 10))) {
        aCallback.notifyDialMMISuccessWithInteger(
          response.statusMessage, response.additionalInformation);
        return;
      }

      
      let array = response.additionalInformation;
      if (Array.isArray(array) && array.length > 0 && typeof array[0] === "string") {
        aCallback.notifyDialMMISuccessWithStrings(response.statusMessage,
                                                  array.length, array);
        return;
      }

      aCallback.notifyDialMMISuccess(response.statusMessage);
    });
  },

  












  _buildMMIRegExp: function() {
    
    
    
    
    
    
    
    
    

    
    let procedure = "(\\*[*#]?|##?)";

    
    
    let serviceCode = "(\\d{2,3})";

    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    let si = "\\*([^*#]*)";
    let allSi = "";
    for (let i = 0; i < 4; ++i) {
      allSi = "(?:" + si + allSi + ")?";
    }

    let fullmmi = "(" + procedure + serviceCode + allSi + "#)";

    
    let dialString = "([^#]*)";

    return new RegExp(fullmmi + dialString);
  },

  


  _getMMIRegExp: function() {
    if (!this._mmiRegExp) {
      this._mmiRegExp = this._buildMMIRegExp();
    }

    return this._mmiRegExp;
  },

  


  _isPoundString: function(aMmiString) {
    return (aMmiString.charAt(aMmiString.length - 1) === "#");
  },

  


  _isShortString: function(aMmiString, hasCalls) {
    if (aMmiString.length > 2) {
      return false;
    }

    if (hasCalls) {
      return true;
    }

    
    
    
    if (this._isEmergencyNumber(aMmiString) ||
        (aMmiString.length == 2) && (aMmiString.charAt(0) === '1')) {
      return false;
    }

    return true;
  },

  


  _parseMMI: function(aMmiString, hasCalls) {
    if (!aMmiString) {
      return null;
    }

    let matches = this._getMMIRegExp().exec(aMmiString);
    if (matches) {
      return {
        fullMMI: matches[MMI_MATCH_GROUP_FULL_MMI],
        procedure: matches[MMI_MATCH_GROUP_PROCEDURE],
        serviceCode: matches[MMI_MATCH_GROUP_SERVICE_CODE],
        sia: matches[MMI_MATCH_GROUP_SIA],
        sib: matches[MMI_MATCH_GROUP_SIB],
        sic: matches[MMI_MATCH_GROUP_SIC],
        pwd: matches[MMI_MATCH_GROUP_PWD_CONFIRM],
        dialNumber: matches[MMI_MATCH_GROUP_DIALING_NUMBER]
      };
    }

    if (this._isPoundString(aMmiString) ||
        this._isShortString(aMmiString, hasCalls)) {
      return {
        fullMMI: aMmiString
      };
    }

    return null;
  },

  _serviceCodeToKeyString: function(aServiceCode) {
    switch (aServiceCode) {
      case RIL.MMI_SC_CFU:
      case RIL.MMI_SC_CF_BUSY:
      case RIL.MMI_SC_CF_NO_REPLY:
      case RIL.MMI_SC_CF_NOT_REACHABLE:
      case RIL.MMI_SC_CF_ALL:
      case RIL.MMI_SC_CF_ALL_CONDITIONAL:
        return RIL.MMI_KS_SC_CALL_FORWARDING;
      case RIL.MMI_SC_PIN:
        return RIL.MMI_KS_SC_PIN;
      case RIL.MMI_SC_PIN2:
        return RIL.MMI_KS_SC_PIN2;
      case RIL.MMI_SC_PUK:
        return RIL.MMI_KS_SC_PUK;
      case RIL.MMI_SC_PUK2:
        return RIL.MMI_KS_SC_PUK2;
      case RIL.MMI_SC_IMEI:
        return RIL.MMI_KS_SC_IMEI;
      case RIL.MMI_SC_CLIP:
        return RIL.MMI_KS_SC_CLIP;
      case RIL.MMI_SC_CLIR:
        return RIL.MMI_KS_SC_CLIR;
      case RIL.MMI_SC_BAOC:
      case RIL.MMI_SC_BAOIC:
      case RIL.MMI_SC_BAOICxH:
      case RIL.MMI_SC_BAIC:
      case RIL.MMI_SC_BAICr:
      case RIL.MMI_SC_BA_ALL:
      case RIL.MMI_SC_BA_MO:
      case RIL.MMI_SC_BA_MT:
        return RIL.MMI_KS_SC_CALL_BARRING;
      case RIL.MMI_SC_CALL_WAITING:
        return RIL.MMI_KS_SC_CALL_WAITING;
      default:
        return RIL.MMI_KS_SC_USSD;
    }
  },

  hangUp: function(aClientId, aCallIndex) {
    let parentId = this._currentCalls[aClientId][aCallIndex].parentId;
    if (parentId) {
      
      
      this.hangUp(aClientId, parentId);
    } else {
      this._sendToRilWorker(aClientId, "hangUp", { callIndex: aCallIndex });
    }
  },

  startTone: function(aClientId, aDtmfChar) {
    this._sendToRilWorker(aClientId, "startTone", { dtmfChar: aDtmfChar });
  },

  stopTone: function(aClientId) {
    this._sendToRilWorker(aClientId, "stopTone");
  },

  answerCall: function(aClientId, aCallIndex) {
    this._sendToRilWorker(aClientId, "answerCall", { callIndex: aCallIndex });
  },

  rejectCall: function(aClientId, aCallIndex) {
    this._sendToRilWorker(aClientId, "rejectCall", { callIndex: aCallIndex });
  },

  holdCall: function(aClientId, aCallIndex) {
    let call = this._currentCalls[aClientId][aCallIndex];
    if (!call || !call.isSwitchable) {
      
      
      return;
    }

    this._sendToRilWorker(aClientId, "holdCall", { callIndex: aCallIndex });
  },

  resumeCall: function(aClientId, aCallIndex) {
    let call = this._currentCalls[aClientId][aCallIndex];
    if (!call || !call.isSwitchable) {
      
      
      return;
    }

    this._sendToRilWorker(aClientId, "resumeCall", { callIndex: aCallIndex });
  },

  conferenceCall: function(aClientId) {
    let indexes = Object.keys(this._currentCalls[aClientId]);
    if (indexes.length < 2) {
      
      
      return;
    }

    for (let i = 0; i < indexes.length; ++i) {
      let call = this._currentCalls[aClientId][indexes[i]];
      if (!call.isMergeable) {
        return;
      }
    }

    function onCdmaConferenceCallSuccess() {
      let indexes = Object.keys(this._currentCalls[aClientId]);
      if (indexes.length < 2) {
        return;
      }

      for (let i = 0; i < indexes.length; ++i) {
        let call = this._currentCalls[aClientId][indexes[i]];
        call.state = RIL.CALL_STATE_ACTIVE;
        call.isConference = true;
        this.notifyCallStateChanged(aClientId, call);
      }
      this.notifyConferenceCallStateChanged(RIL.CALL_STATE_ACTIVE);
    }

    this._sendToRilWorker(aClientId, "conferenceCall", null, response => {
      if (!response.success) {
        this._notifyAllListeners("notifyConferenceError", [response.errorName,
                                                           response.errorMsg]);
        return;
      }

      if (response.isCdma) {
        onCdmaConferenceCallSuccess.call(this);
      }
    });
  },

  separateCall: function(aClientId, aCallIndex) {
    let call = this._currentCalls[aClientId][aCallIndex];
    if (!call || !call.isConference) {
      
      
      return;
    }

    let parentId = call.parentId;
    if (parentId) {
      this.separateCall(aClientId, parentId);
      return;
    }

    function onCdmaSeparateCallSuccess() {
      
      let call = this._currentCalls[aClientId][aCallIndex];
      if (!call || !call.isConference) {
        return;
      }

      let childId = call.childId;
      if (!childId) {
        return;
      }

      let childCall = this._currentCalls[aClientId][childId];
      this.notifyCallDisconnected(aClientId, childCall);
    }

    this._sendToRilWorker(aClientId, "separateCall", { callIndex: aCallIndex },
                          response => {
      if (!response.success) {
        this._notifyAllListeners("notifyConferenceError", [response.errorName,
                                                           response.errorMsg]);
        return;
      }

      if (response.isCdma) {
        onCdmaSeparateCallSuccess.call(this);
      }
    });
  },

  hangUpConference: function(aClientId, aCallback) {
    this._sendToRilWorker(aClientId, "hangUpConference", null, response => {
      if (!response.success) {
        aCallback.notifyError(response.errorMsg);
      } else {
        aCallback.notifySuccess();
      }
    });
  },

  holdConference: function(aClientId) {
    this._sendToRilWorker(aClientId, "holdConference");
  },

  resumeConference: function(aClientId) {
    this._sendToRilWorker(aClientId, "resumeConference");
  },

  sendUSSD: function(aClientId, aUssd, aCallback) {
    this._sendToRilWorker(aClientId, "sendUSSD",
                          { ussd: aUssd, checkSession: true },
                          response => {
      if (!response.success) {
        aCallback.notifyError(response.errorMsg);
      } else {
        aCallback.notifySuccess();
      }
    });
  },

  get microphoneMuted() {
    return gAudioService.microphoneMuted;
  },

  set microphoneMuted(aMuted) {
    gAudioService.microphoneMuted = aMuted;
  },

  get speakerEnabled() {
    return gAudioService.speakerEnabled;
  },

  set speakerEnabled(aEnabled) {
    gAudioService.speakerEnabled = aEnabled;
  },

  



  notifyAudioStateChanged: function(aClientId, aState) {
    this._audioStates[aClientId] = aState;

    let audioState = aState;
    for (let i = 0; i < this._numClients; ++i) {
      audioState = Math.max(audioState, this._audioStates[i]);
    }

    this._updateAudioState(audioState);
  },

  


  notifyCallDisconnected: function(aClientId, aCall) {
    if (DEBUG) debug("handleCallDisconnected: " + JSON.stringify(aCall));

    aCall.clientId = aClientId;
    aCall.state = nsITelephonyService.CALL_STATE_DISCONNECTED;
    aCall.isEmergency = this._isEmergencyNumber(aCall.number);
    let duration = ("started" in aCall && typeof aCall.started == "number") ?
      new Date().getTime() - aCall.started : 0;
    let data = {
      number: aCall.number,
      serviceId: aClientId,
      emergency: aCall.isEmergency,
      duration: duration,
      direction: aCall.isOutgoing ? "outgoing" : "incoming",
      hangUpLocal: aCall.hangUpLocal
    };

    if (this._cdmaCallWaitingNumber != null) {
      data.secondNumber = this._cdmaCallWaitingNumber;
      this._cdmaCallWaitingNumber = null;
    }

    gSystemMessenger.broadcastMessage("telephony-call-ended", data);

    let manualConfStateChange = false;
    let childId = this._currentCalls[aClientId][aCall.callIndex].childId;
    if (childId) {
      
      let childCall = this._currentCalls[aClientId][childId];
      this.notifyCallDisconnected(aClientId, childCall);
    } else {
      let parentId = this._currentCalls[aClientId][aCall.callIndex].parentId;
      if (parentId) {
        let parentCall = this._currentCalls[aClientId][parentId];
        
        delete parentCall.childId;
        if (parentCall.isConference) {
          
          
          manualConfStateChange = true;
          parentCall.isConference = false;
          parentCall.isSwitchable = true;
          parentCall.isMergeable = true;
          aCall.isConference = false;
          this.notifyCallStateChanged(aClientId, parentCall, true);
        }
      }
    }

    this._updateActiveCall(aCall);

    if (!aCall.failCause ||
        aCall.failCause === RIL.GECKO_CALL_ERROR_NORMAL_CALL_CLEARING) {
      this._notifyAllListeners("callStateChanged", [aClientId,
                                                    aCall.callIndex,
                                                    aCall.state,
                                                    aCall.number,
                                                    aCall.numberPresentation,
                                                    aCall.name,
                                                    aCall.namePresentation,
                                                    aCall.isOutgoing,
                                                    aCall.isEmergency,
                                                    aCall.isConference,
                                                    aCall.isSwitchable,
                                                    aCall.isMergeable]);
    } else {
      this._notifyAllListeners("notifyError",
                               [aClientId, aCall.callIndex, aCall.failCause]);
    }
    delete this._currentCalls[aClientId][aCall.callIndex];

    if (manualConfStateChange) {
      this.notifyConferenceCallStateChanged(RIL.CALL_STATE_UNKNOWN);
    }
  },

  





  notifyCallRing: function() {
    
    
    this._acquireCallRingWakeLock();

    gSystemMessenger.broadcastMessage("telephony-new-call", {});
  },

  



  notifyCallStateChanged: function(aClientId, aCall, aSkipStateConversion) {
    if (DEBUG) debug("handleCallStateChange: " + JSON.stringify(aCall));

    if (!aSkipStateConversion) {
      aCall.state = this._convertRILCallState(aCall.state);
    }

    if (aCall.state == nsITelephonyService.CALL_STATE_DIALING) {
      gSystemMessenger.broadcastMessage("telephony-new-call", {});
    }

    aCall.clientId = aClientId;
    this._updateActiveCall(aCall);

    function pick(arg, defaultValue) {
      return typeof arg !== 'undefined' ? arg : defaultValue;
    }

    let call = this._currentCalls[aClientId][aCall.callIndex];
    if (call) {
      call.state = aCall.state;
      call.number = aCall.number;
      call.isConference = aCall.isConference;
      call.isEmergency = this._isEmergencyNumber(aCall.number);
      call.isSwitchable = pick(aCall.isSwitchable, call.isSwitchable);
      call.isMergeable = pick(aCall.isMergeable, call.isMergeable);
    } else {
      call = aCall;
      call.isEmergency = pick(aCall.isEmergency, this._isEmergencyNumber(aCall.number));
      call.isSwitchable = pick(aCall.isSwitchable, true);
      call.isMergeable = pick(aCall.isMergeable, true);
      call.name = pick(aCall.name, "");
      call.numberPresentaation = pick(aCall.numberPresentation, nsITelephonyService.CALL_PRESENTATION_ALLOWED);
      call.namePresentaation = pick(aCall.namePresentation, nsITelephonyService.CALL_PRESENTATION_ALLOWED);

      this._currentCalls[aClientId][aCall.callIndex] = call;
    }

    
    if (this._cachedDialRequest && !this._getOneActiveCall()) {
      if (DEBUG) debug("All calls held. Perform the cached dial request.");

      let request = this._cachedDialRequest;
      this._sendDialCallRequest(request.clientId, request.options, request.callback);
      this._cachedDialRequest = null;
    }

    this._notifyAllListeners("callStateChanged", [aClientId,
                                                  call.callIndex,
                                                  call.state,
                                                  call.number,
                                                  call.numberPresentation,
                                                  call.name,
                                                  call.namePresentation,
                                                  call.isOutgoing,
                                                  call.isEmergency,
                                                  call.isConference,
                                                  call.isSwitchable,
                                                  call.isMergeable]);
  },

  notifyCdmaCallWaiting: function(aClientId, aCall) {
    
    
    this._acquireCallRingWakeLock();

    let call = this._currentCalls[aClientId][CDMA_SECOND_CALL_INDEX];
    if (call) {
      
      
      this.notifyCallDisconnected(aClientId, call);
    }

    this._cdmaCallWaitingNumber = aCall.number;

    this._notifyAllListeners("notifyCdmaCallWaiting", [aClientId,
                                                       aCall.number,
                                                       aCall.numberPresentation,
                                                       aCall.name,
                                                       aCall.namePresentation]);
  },

  notifySupplementaryService: function(aClientId, aCallIndex, aNotification) {
    let notification = this._convertRILSuppSvcNotification(aNotification);
    this._notifyAllListeners("supplementaryServiceNotification",
                             [aClientId, aCallIndex, notification]);
  },

  notifyConferenceCallStateChanged: function(aState) {
    if (DEBUG) debug("handleConferenceCallStateChanged: " + aState);
    aState = this._convertRILCallState(aState);
    this._notifyAllListeners("conferenceCallStateChanged", [aState]);
  },

  notifyUssdReceived: function(aClientId, aMessage, aSessionEnded) {
    if (DEBUG) {
      debug("notifyUssdReceived for " + aClientId + ": " +
            aMessage + " (sessionEnded : " + aSessionEnded + ")");
    }

    let info = {
      serviceId: aClientId,
      message: aMessage,
      sessionEnded: aSessionEnded
    };

    gSystemMessenger.broadcastMessage("ussd-received", info);

    gGonkMobileConnectionService.notifyUssdReceived(aClientId, aMessage,
                                                    aSessionEnded);
  },

  dialMMI: function(aClientId, aMmiString, aCallback) {
    let mmi = this._parseMMI(aMmiString, this._hasCalls(aClientId));
    this._dialMMI(aClientId, mmi, aCallback, false);
  },

  



  observe: function(aSubject, aTopic, aData) {
    switch (aTopic) {
      case NS_PREFBRANCH_PREFCHANGE_TOPIC_ID:
        if (aData === kPrefRilDebuggingEnabled) {
          this._updateDebugFlag();
        } else if (aData === kPrefDefaultServiceId) {
          this.defaultServiceId = this._getDefaultServiceId();
        }
        break;

      case NS_XPCOM_SHUTDOWN_OBSERVER_ID:
        
        this._releaseCallRingWakeLock();

        Services.obs.removeObserver(this, NS_XPCOM_SHUTDOWN_OBSERVER_ID);
        break;
    }
  }
};







function USSDReceivedWrapper() {
  if (DEBUG) debug("USSDReceivedWrapper()");
}
USSDReceivedWrapper.prototype = {
  
  wrapMessage: function(aMessage, aWindow) {
    if (DEBUG) debug("wrapMessage: " + JSON.stringify(aMessage));

    let session = aMessage.sessionEnded ? null :
      new aWindow.USSDSession(aMessage.serviceId);

    let event = new aWindow.USSDReceivedEvent("ussdreceived", {
      serviceId: aMessage.serviceId,
      message: aMessage.message,
      sessionEnded: aMessage.sessionEnded,
      session: session
    });

    return event;
  },

  classDescription: "USSDReceivedWrapper",
  classID: Components.ID("{d03684ed-ede4-4210-8206-f4f32772d9f5}"),
  contractID: "@mozilla.org/dom/system-messages/wrapper/ussd-received;1",
  QueryInterface: XPCOMUtils.generateQI([Ci.nsISystemMessagesWrapper])
};

this.NSGetFactory = XPCOMUtils.generateNSGetFactory([TelephonyService,
                                                    USSDReceivedWrapper]);
