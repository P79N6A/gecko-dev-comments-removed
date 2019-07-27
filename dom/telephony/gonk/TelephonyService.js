




"use strict";

const {classes: Cc, interfaces: Ci, utils: Cu, results: Cr} = Components;

Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://gre/modules/Services.jsm");
Cu.import("resource://gre/modules/Promise.jsm");


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
const TELEPHONYCALLINFO_CID =
  Components.ID("{d9e8b358-a02c-4cf3-9fc7-816c2e8d46e4}");

const NS_XPCOM_SHUTDOWN_OBSERVER_ID = "xpcom-shutdown";

const NS_PREFBRANCH_PREFCHANGE_TOPIC_ID = "nsPref:changed";

const kPrefRilNumRadioInterfaces = "ril.numRadioInterfaces";
const kPrefRilDebuggingEnabled = "ril.debugging.enabled";
const kPrefDefaultServiceId = "dom.telephony.defaultServiceId";

const nsITelephonyAudioService = Ci.nsITelephonyAudioService;
const nsITelephonyService = Ci.nsITelephonyService;
const nsIMobileConnection = Ci.nsIMobileConnection;

const CALL_WAKELOCK_TIMEOUT = 5000;



const CDMA_FIRST_CALL_INDEX = 1;
const CDMA_SECOND_CALL_INDEX = 2;

const DIAL_ERROR_INVALID_STATE_ERROR = "InvalidStateError";
const DIAL_ERROR_OTHER_CONNECTION_IN_USE = "OtherConnectionInUse";
const DIAL_ERROR_BAD_NUMBER = RIL.GECKO_CALL_ERROR_BAD_NUMBER;
const DIAL_ERROR_RADIO_NOT_AVAILABLE = RIL.GECKO_ERROR_RADIO_NOT_AVAILABLE;

const TONES_GAP_DURATION = 70;

let DEBUG;
function debug(s) {
  dump("TelephonyService: " + s + "\n");
}


XPCOMUtils.defineLazyGetter(this, "gRadioInterfaceLayer", function() {
  let ril = { numRadioInterfaces: 0 };
  try {
    ril = Cc["@mozilla.org/ril;1"].getService(Ci.nsIRadioInterfaceLayer);
  } catch(e) {}
  return ril;
});


XPCOMUtils.defineLazyServiceGetter(this, "gPowerManagerService",
                                   "@mozilla.org/power/powermanagerservice;1",
                                   "nsIPowerManagerService");


XPCOMUtils.defineLazyServiceGetter(this, "gTelephonyMessenger",
                                   "@mozilla.org/ril/system-messenger-helper;1",
                                   "nsITelephonyMessenger");


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


XPCOMUtils.defineLazyGetter(this, "gDialNumberUtils", function() {
  let ns = {};
  Cu.import("resource://gre/modules/DialNumberUtils.jsm", ns);
  return ns.DialNumberUtils;
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
  action: nsIMobileConnection.CALL_FORWARD_ACTION_UNKNOWN,
  reason: nsIMobileConnection.CALL_FORWARD_REASON_UNKNOWN,
  number: null,
  timeSeconds: -1,
  serviceClass: nsIMobileConnection.ICC_SERVICE_CLASS_NONE
};

function TelephonyCallInfo(aCall) {
  this.clientId = aCall.clientId;
  this.callIndex = aCall.callIndex;
  this.callState = aCall.state;
  this.number = aCall.number;
  this.numberPresentation = aCall.numberPresentation;
  this.name = aCall.name;
  this.namePresentation = aCall.namePresentation;
  this.isOutgoing = aCall.isOutgoing;
  this.isEmergency = aCall.isEmergency;
  this.isConference = aCall.isConference;
  this.isSwitchable = aCall.isSwitchable;
  this.isMergeable = aCall.isMergeable;
}
TelephonyCallInfo.prototype = {
  QueryInterface: XPCOMUtils.generateQI([Ci.nsITelephonyCallInfo]),
  classID: TELEPHONYCALLINFO_CID,
  classInfo: XPCOMUtils.generateCI({
    classID:          TELEPHONYCALLINFO_CID,
    classDescription: "TelephonyCallInfo",
    interfaces:       [Ci.nsITelephonyCallInfo]
  }),

  

  clientId: 0,
  callIndex: 0,
  callState: nsITelephonyService.CALL_STATE_UNKNOWN,
  number: "",
  numberPresentation: nsITelephonyService.CALL_PRESENTATION_ALLOWED,
  name: "",
  namePresentation: nsITelephonyService.CALL_PRESENTATION_ALLOWED,
  isOutgoing: true,
  isEmergency: false,
  isConference: false,
  isSwitchable: true,
  isMergeable: true
};

function Call(aClientId, aCallIndex) {
  this.clientId = aClientId;
  this.callIndex = aCallIndex;
}
Call.prototype = {
  clientId: 0,
  callIndex: 0,
  state: nsITelephonyService.CALL_STATE_UNKNOWN,
  number: "",
  numberPresentation: nsITelephonyService.CALL_PRESENTATION_ALLOWED,
  name: "",
  namePresentation: nsITelephonyService.CALL_PRESENTATION_ALLOWED,
  isOutgoing: true,
  isEmergency: false,
  isConference: false,
  isSwitchable: true,
  isMergeable: true,
  started: null
};

function MobileConnectionListener() {}
MobileConnectionListener.prototype = {
  QueryInterface: XPCOMUtils.generateQI([Ci.nsIMobileConnectionListener]),

  

  notifyVoiceChanged: function() {},
  notifyDataChanged: function() {},
  notifyDataError: function(message) {},
  notifyCFStateChanged: function(action, reason, number, timeSeconds, serviceClass) {},
  notifyEmergencyCbModeChanged: function(active, timeoutMs) {},
  notifyOtaStatusChanged: function(status) {},
  notifyRadioStateChanged: function() {},
  notifyClirModeChanged: function(mode) {},
  notifyLastKnownNetworkChanged: function() {},
  notifyLastKnownHomeNetworkChanged: function() {},
  notifyNetworkSelectionModeChanged: function() {}
};

function TelephonyService() {
  this._numClients = gRadioInterfaceLayer.numRadioInterfaces;
  this._listeners = [];

  this._isDialing = false;
  this._cachedDialRequest = null;
  this._currentCalls = {};
  this._currentConferenceState = nsITelephonyService.CALL_STATE_UNKNOWN;
  this._audioStates = [];

  this._cdmaCallWaitingNumber = null;

  this._updateDebugFlag();
  this.defaultServiceId = this._getDefaultServiceId();

  Services.prefs.addObserver(kPrefRilDebuggingEnabled, this, false);
  Services.prefs.addObserver(kPrefDefaultServiceId, this, false);

  Services.obs.addObserver(this, NS_XPCOM_SHUTDOWN_OBSERVER_ID, false);

  for (let i = 0; i < this._numClients; ++i) {
    this._audioStates[i] = nsITelephonyAudioService.PHONE_STATE_NORMAL;
    this._currentCalls[i] = {};
    this._enumerateCallsForClient(i);
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

  _isGsmTechGroup: function(aType) {
    switch (aType) {
      case null:  
      case "gsm":
      case "gprs":
      case "edge":
      case "umts":
      case "hsdpa":
      case "hsupa":
      case "hspa":
      case "hspa+":
      case "lte":
        return true;
      default:
        return false;
    }
  },

  _isCdmaClient: function(aClientId) {
    let type = gGonkMobileConnectionService.getItemByServiceId(aClientId).voice.type;
    return !this._isGsmTechGroup(type);
  },

  _isEmergencyOnly: function(aClientId) {
    return gGonkMobileConnectionService.getItemByServiceId(aClientId).voice.emergencyCallsOnly;
  },

  _isRadioOn: function(aClientId) {
    let connection = gGonkMobileConnectionService.getItemByServiceId(aClientId);
    return connection.radioState === nsIMobileConnection.MOBILE_RADIO_STATE_ENABLED;
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

  _computeAudioStateForClient: function(aClientId) {
    let indexes = Object.keys(this._currentCalls[aClientId]);
    if (!indexes.length) {
      return nsITelephonyAudioService.PHONE_STATE_NORMAL;
    }

    let firstCall = this._currentCalls[aClientId][indexes[0]];
    if (indexes.length === 1 &&
        firstCall.state === nsITelephonyService.CALL_STATE_INCOMING) {
      return nsITelephonyAudioService.PHONE_STATE_RINGTONE;
    }

    return nsITelephonyAudioService.PHONE_STATE_IN_CALL;
  },

  _updateAudioState: function(aClientId) {
    this._audioStates[aClientId] = this._computeAudioStateForClient(aClientId);

    if (this._audioStates.some(state => state === nsITelephonyAudioService.PHONE_STATE_IN_CALL)) {
      gAudioService.setPhoneState(nsITelephonyAudioService.PHONE_STATE_IN_CALL);
    } else if (this._audioStates.some(state => state === nsITelephonyAudioService.PHONE_STATE_RINGTONE)) {
      gAudioService.setPhoneState(nsITelephonyAudioService.PHONE_STATE_RINGTONE);
    } else {
      gAudioService.setPhoneState(nsITelephonyAudioService.PHONE_STATE_NORMAL);
    }
  },

  _formatInternationalNumber: function(aNumber, aToa) {
    if (aNumber && aToa == RIL.TOA_INTERNATIONAL && aNumber[0] != "+") {
      return "+" + aNumber;
    }

    return aNumber;
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

    this._sendToRilWorker(aClientId, "getCurrentCalls", null, response => {
      if (response.errorMsg) {
        return;
      }

      
      this._currentCalls[aClientId] = {};

      for (let i in response.calls) {
        let call = this._currentCalls[aClientId][i] = new Call(aClientId, i);
        this._updateCallFromRil(call, response.calls[i]);
      }
    });
  },

  





  _isTemporaryCLIR: function(aMmi) {
    return (aMmi && aMmi.serviceCode === RIL.MMI_SC_CLIR) && aMmi.dialNumber;
  },

  





  _procedureToCLIRMode: function(aProcedure) {
    
    
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
        let callInfo = new TelephonyCallInfo(call);
        aListener.enumerateCallState(callInfo);
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

  


  _isActive: function(aClientId) {
    for (let index in this._currentCalls[aClientId]) {
      let call = this._currentCalls[aClientId][index];
      if (call.state === nsITelephonyService.CALL_STATE_CONNECTED) {
        return true;
      }
    }
    return false;
  },

  




  dial: function(aClientId, aNumber, aIsDialEmergency, aCallback) {
    if (DEBUG) debug("Dialing " + (aIsDialEmergency ? "emergency " : "")
                     + aNumber + ", clientId: " + aClientId);

    
    
    if (!aIsDialEmergency) {
      aNumber = gPhoneNumberUtils.normalize(aNumber);
    }

    
    
    if (!gPhoneNumberUtils.isPlainPhoneNumber(aNumber)) {
      if (DEBUG) debug("Error: Number '" + aNumber + "' is not viable. Drop.");
      aCallback.notifyError(DIAL_ERROR_BAD_NUMBER);
      return;
    }

    let isEmergencyNumber = gDialNumberUtils.isEmergency(aNumber);

    
    if (aIsDialEmergency && !isEmergencyNumber) {
      if (!this._isRadioOn(aClientId)) {
        if (DEBUG) debug("Error: Radio is off. Drop.");
        aCallback.notifyError(DIAL_ERROR_RADIO_NOT_AVAILABLE);
        return;
      }

      if (DEBUG) debug("Error: Dial a non-emergency by dialEmergency. Drop.");
      aCallback.notifyError(DIAL_ERROR_BAD_NUMBER);
      return;
    }

    if (isEmergencyNumber) {
      this._dialCall(aClientId, aNumber, undefined, aCallback);
      return;
    }

    
    if (this._isCdmaClient(aClientId)) {
      this._dialCall(aClientId, aNumber, undefined, aCallback);
      return;
    }

    let mmi = gDialNumberUtils.parseMMI(aNumber);
    if (mmi) {
      if (this._isTemporaryCLIR(mmi)) {
        this._dialCall(aClientId, mmi.dialNumber,
                       this._procedureToCLIRMode(mmi.procedure), aCallback);
      } else {
        this._dialMMI(aClientId, mmi, aCallback);
      }
    } else {
      if (aNumber[aNumber.length - 1] === "#") {  
        this._dialMMI(aClientId, {fullMMI: aNumber}, aCallback);
      } else if (aNumber.length <= 2) {  
        if (this._hasCalls(aClientId)) {
          this._dialInCallMMI(aClientId, aNumber, aCallback);
        } else if (aNumber.length === 2 && aNumber[0] === "1") {
          this._dialCall(aClientId, aNumber, undefined, aCallback);
        } else {
          this._dialMMI(aClientId, {fullMMI: aNumber}, aCallback);
        }
      } else {
        this._dialCall(aClientId, aNumber, undefined, aCallback);
      }
    }
  },

  
  _dialInCallMMI: function(aClientId, aNumber, aCallback) {
    let mmiCallback = {
      notifyError: () => aCallback.notifyDialMMIError(RIL.MMI_ERROR_KS_ERROR),
      notifySuccess: () => aCallback.notifyDialMMISuccess(RIL.MMI_SM_KS_CALL_CONTROL)
    };

    if (aNumber === "0") {
      aCallback.notifyDialMMI(RIL.MMI_KS_SC_CALL);
      this._hangUpBackground(aClientId, mmiCallback);
    } else if (aNumber === "1") {
      aCallback.notifyDialMMI(RIL.MMI_KS_SC_CALL);
      this._hangUpForeground(aClientId, mmiCallback);
    } else if (aNumber[0] === "1" && aNumber.length === 2) {
      aCallback.notifyDialMMI(RIL.MMI_KS_SC_CALL);
      this.hangUpCall(aClientId, parseInt(aNumber[1]), mmiCallback);
    } else if (aNumber === "2") {
      aCallback.notifyDialMMI(RIL.MMI_KS_SC_CALL);
      this._switchActiveCall(aClientId, mmiCallback);
    } else if (aNumber[0] === "2" && aNumber.length === 2) {
      aCallback.notifyDialMMI(RIL.MMI_KS_SC_CALL);
      this._separateCallGsm(aClientId, parseInt(aNumber[1]), mmiCallback);
    } else if (aNumber === "3") {
      aCallback.notifyDialMMI(RIL.MMI_KS_SC_CALL);
      this._conferenceCallGsm(aClientId, mmiCallback);
    } else {
      this._dialCall(aClientId, aNumber, undefined, aCallback);
    }
  },

  _dialCall: function(aClientId, aNumber, aClirMode = RIL.CLIR_DEFAULT,
                      aCallback) {
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

    let isEmergency = gDialNumberUtils.isEmergency(aNumber);

    if (!isEmergency) {
      if (!this._isRadioOn(aClientId)) {
        if (DEBUG) debug("Error: Dial a normal call when radio off. Drop");
        aCallback.notifyError(DIAL_ERROR_RADIO_NOT_AVAILABLE);
        return;
      }

      if (this._isEmergencyOnly(aClientId)) {
        if (DEBUG) debug("Error: Dial a normal call when emergencyCallsOnly. Drop");
        aCallback.notifyError(DIAL_ERROR_BAD_NUMBER);
        return;
      }
    }

    if (isEmergency) {
      
      aClientId = gRadioInterfaceLayer.getClientIdForEmergencyCall() ;
      if (aClientId === -1) {
        if (DEBUG) debug("Error: No client is avaialble for emergency call.");
        aCallback.notifyError(DIAL_ERROR_INVALID_STATE_ERROR);
        return;
      }

      
      if (!this._isRadioOn(aClientId)) {
        let connection = gGonkMobileConnectionService.getItemByServiceId(aClientId);
        let listener = new MobileConnectionListener();

        listener.notifyRadioStateChanged = () => {
          if (this._isRadioOn(aClientId)) {
            this._dialCall(aClientId, aNumber, undefined, aCallback);
            connection.unregisterListener(listener);
          }
        };
        connection.registerListener(listener);

        
        connection.setRadioEnabled(true, {
          QueryInterface: XPCOMUtils.generateQI([Ci.nsIMobileConnectionCallback]),
          notifySuccess: () => {},
          notifyError: aErrorMsg => {
            connection.unregisterListener(listener);
            aCallback.notifyError(DIAL_ERROR_RADIO_NOT_AVAILABLE);
          }
        });

        return;
      }
    }

    let options = {
      isEmergency: isEmergency,
      number: aNumber,
      clirMode: aClirMode
    };

    
    if (!this._isActive(aClientId)) {
      this._sendDialCallRequest(aClientId, options, aCallback);
      return;
    }

    
    if (this._isCdmaClient(aClientId)) {
      this._dialCdmaThreeWayCall(aClientId, aNumber, aCallback);
      return;
    }

    
    if (DEBUG) debug("There is an active call. Hold it first before dial.");

    if (this._cachedDialRequest) {
      if (DEBUG) debug("Error: There already is a pending dial request.");
      aCallback.notifyError(DIAL_ERROR_INVALID_STATE_ERROR);
      return;
    }

    this._switchActiveCall(aClientId, {
      QueryInterface: XPCOMUtils.generateQI([Ci.nsITelephonyCallback]),

      notifySuccess: () => {
        this._cachedDialRequest = {
          clientId: aClientId,
          options: options,
          callback: aCallback
        };
      },

      notifyError: (aErrorMsg) => {
        if (DEBUG) debug("Error: Fail to automatically hold the active call.");
        aCallback.notifyError(aErrorMsg);
      }
    });
  },

  _dialCdmaThreeWayCall: function(aClientId, aNumber, aCallback) {
    this._sendToRilWorker(aClientId, "cdmaFlash", { featureStr: aNumber },
                          response => {
      if (!response.success) {
        aCallback.notifyError(response.errorMsg);
        return;
      }

      
      aCallback.notifyDialCallSuccess(aClientId, CDMA_SECOND_CALL_INDEX,
                                      aNumber);

      let childCall = this._currentCalls[aClientId][CDMA_SECOND_CALL_INDEX] =
        new Call(aClientId, CDMA_SECOND_CALL_INDEX);

      childCall.parentId = CDMA_FIRST_CALL_INDEX;
      childCall.state = nsITelephonyService.CALL_STATE_DIALING;
      childCall.number = aNumber;
      childCall.isOutgoing = true;
      childCall.isEmergency = gDialNumberUtils.isEmergency(aNumber);
      childCall.isConference = false;
      childCall.isSwitchable = false;
      childCall.isMergeable = true;

      
      this._handleCallStateChanged(aClientId, [childCall]);

      childCall.state = nsITelephonyService.CALL_STATE_CONNECTED;

      let parentCall = this._currentCalls[aClientId][childCall.parentId];
      parentCall.childId = CDMA_SECOND_CALL_INDEX;
      parentCall.state = nsITelephonyService.CALL_STATE_HELD;
      parentCall.isSwitchable = false;
      parentCall.isMergeable = true;
      this._handleCallStateChanged(aClientId, [childCall, parentCall]);
    });
  },

  _sendDialCallRequest: function(aClientId, aOptions, aCallback) {
    this._isDialing = true;

    this._sendToRilWorker(aClientId, "dial", aOptions, response => {
      this._isDialing = false;

      if (!response.success) {
        this._sendToRilWorker(aClientId, "getFailCause", null, response => {
          aCallback.notifyError(response.failCause);
        });
      } else {
        this._ongoingDial = {
          clientId: aClientId,
          callback: aCallback
        };
      }
    });
  },

  









  _dialMMI: function(aClientId, aMmi, aCallback) {
    let mmiServiceCode = aMmi ?
      this._serviceCodeToKeyString(aMmi.serviceCode) : RIL.MMI_KS_SC_USSD;

    aCallback.notifyDialMMI(mmiServiceCode);

    this._sendToRilWorker(aClientId, "sendMMI",
                          { mmi: aMmi }, response => {
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

      
      if (response.additionalInformation === undefined) {
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
      case RIL.MMI_SC_CHANGE_PASSWORD:
        return RIL.MMI_KS_SC_CHANGE_PASSWORD;
      default:
        return RIL.MMI_KS_SC_USSD;
    }
  },

  







  _defaultCallbackHandler: function(aCallback, aResponse) {
    if (!aResponse.success) {
      aCallback.notifyError(aResponse.errorMsg);
    } else {
      aCallback.notifySuccess();
    }
  },

  _getCallsWithState: function(aClientId, aState) {
    let calls = [];
    for (let i in this._currentCalls[aClientId]) {
      let call = this._currentCalls[aClientId][i];
      if (call.state === aState) {
        calls.push(call);
      }
    }
    return calls;
  },

  




  _updateCallFromRil: function(aCall, aRilCall) {
    aRilCall.state = this._convertRILCallState(aRilCall.state);
    aRilCall.number = this._formatInternationalNumber(aRilCall.number,
                                                      aRilCall.toa);

    let change = false;
    const key = ["state", "number", "numberPresentation", "name",
                 "namePresentation"];

    for (let k of key) {
      if (aCall[k] != aRilCall[k]) {
        aCall[k] = aRilCall[k];
        change = true;
      }
    }

    aCall.isOutgoing = !aRilCall.isMT;
    aCall.isEmergency = gDialNumberUtils.isEmergency(aCall.number);

    if (!aCall.started &&
        aCall.state == nsITelephonyService.CALL_STATE_CONNECTED) {
      aCall.started = new Date().getTime();
    }

    return change;
  },

  





  _detectConference: function(aClientId) {
    
    
    
    

    
    
    
    let connectedCalls = this._getCallsWithState(aClientId, nsITelephonyService.CALL_STATE_CONNECTED);
    let heldCalls = this._getCallsWithState(aClientId, nsITelephonyService.CALL_STATE_HELD);

    if (connectedCalls.length >= 2) {
      return [nsITelephonyService.CALL_STATE_CONNECTED, connectedCalls];
    } else if (heldCalls.length >= 2) {
      return [nsITelephonyService.CALL_STATE_HELD, heldCalls];
    }

    return [nsITelephonyService.CALL_STATE_UNKNOWN, null];
  },

  




  _updateConference: function(aClientId) {
    let [newConferenceState, conferenceCalls] = this._detectConference(aClientId);
    if (DEBUG) debug("Conference state: " + newConferenceState);

    let changedCalls = [];
    let conference = new Set(conferenceCalls);

    for (let i in this._currentCalls[aClientId]) {
      let call = this._currentCalls[aClientId][i];
      let isConference = conference.has(call);
      if (call.isConference != isConference) {
        call.isConference = isConference;
        changedCalls.push(call);
      }
    }

    return [newConferenceState, changedCalls];
  },

  sendTones: function(aClientId, aDtmfChars, aPauseDuration, aToneDuration,
                      aCallback) {
    let timer = Cc["@mozilla.org/timer;1"].createInstance(Ci.nsITimer);
    let tones = aDtmfChars;
    let playTone = (tone) => {
      this._sendToRilWorker(aClientId, "startTone", { dtmfChar: tone }, response => {
        if (!response.success) {
          aCallback.notifyError(response.errorMsg);
          return;
        }

        timer.initWithCallback(() => {
          this.stopTone();
          timer.initWithCallback(() => {
            if (tones.length === 1) {
              aCallback.notifySuccess();
            } else {
              tones = tones.substr(1);
              playTone(tones[0]);
            }
          }, TONES_GAP_DURATION, Ci.nsITimer.TYPE_ONE_SHOT);
        }, aToneDuration, Ci.nsITimer.TYPE_ONE_SHOT);
      });
    };

    timer.initWithCallback(() => {
      playTone(tones[0]);
    }, aPauseDuration, Ci.nsITimer.TYPE_ONE_SHOT);
  },

  startTone: function(aClientId, aDtmfChar) {
    this._sendToRilWorker(aClientId, "startTone", { dtmfChar: aDtmfChar });
  },

  stopTone: function(aClientId) {
    this._sendToRilWorker(aClientId, "stopTone");
  },

  answerCall: function(aClientId, aCallIndex, aCallback) {
    let call = this._currentCalls[aClientId][aCallIndex];
    if (!call || call.state != nsITelephonyService.CALL_STATE_INCOMING) {
      aCallback.notifyError(RIL.GECKO_ERROR_GENERIC_FAILURE);
      return;
    }

    let callNum = Object.keys(this._currentCalls[aClientId]).length;
    if (callNum !== 1) {
      this._switchActiveCall(aClientId, aCallback);
    } else {
      this._sendToRilWorker(aClientId, "answerCall", null,
                            this._defaultCallbackHandler.bind(this, aCallback));
    }
  },

  rejectCall: function(aClientId, aCallIndex, aCallback) {
    if (this._isCdmaClient(aClientId)) {
      this._hangUpBackground(aClientId, aCallback);
      return;
    }

    let call = this._currentCalls[aClientId][aCallIndex];
    if (!call || call.state != nsITelephonyService.CALL_STATE_INCOMING) {
      aCallback.notifyError(RIL.GECKO_ERROR_GENERIC_FAILURE);
      return;
    }

    let callNum = Object.keys(this._currentCalls[aClientId]).length;
    if (callNum !== 1) {
      this._hangUpBackground(aClientId, aCallback);
    } else {
      call.hangUpLocal = true;
      this._sendToRilWorker(aClientId, "udub", null,
                            this._defaultCallbackHandler.bind(this, aCallback));
    }
  },

  _hangUpForeground: function(aClientId, aCallback) {
    let calls = this._getCallsWithState(aClientId, nsITelephonyService.CALL_STATE_CONNECTED);
    calls.forEach(call => call.hangUpLocal = true);

    this._sendToRilWorker(aClientId, "hangUpForeground", null,
                          this._defaultCallbackHandler.bind(this, aCallback));
  },

  _hangUpBackground: function(aClientId, aCallback) {
    
    
    let waitingCalls = this._getCallsWithState(aClientId, nsITelephonyService.CALL_STATE_INCOMING);
    let heldCalls = this._getCallsWithState(aClientId, nsITelephonyService.CALL_STATE_HELD);

    if (waitingCalls.length) {
      waitingCalls.forEach(call => call.hangUpLocal = true);
    } else {
      heldCalls.forEach(call => call.hangUpLocal = true);
    }

    this._sendToRilWorker(aClientId, "hangUpBackground", null,
                          this._defaultCallbackHandler.bind(this, aCallback));
  },

  hangUpCall: function(aClientId, aCallIndex, aCallback) {
    
    
    aCallIndex = this._currentCalls[aClientId][aCallIndex].parentId || aCallIndex;

    let call = this._currentCalls[aClientId][aCallIndex];
    if (call.state === nsITelephonyService.CALL_STATE_HELD) {
      this._hangUpBackground(aClientId, aCallback);
      return;
    }

    call.hangUpLocal = true;
    this._sendToRilWorker(aClientId, "hangUpCall", { callIndex: aCallIndex },
                          this._defaultCallbackHandler.bind(this, aCallback));
  },

  _switchCall: function(aClientId, aCallIndex, aCallback, aRequiredState) {
    let call = this._currentCalls[aClientId][aCallIndex];
    if (!call) {
      aCallback.notifyError(RIL.GECKO_ERROR_GENERIC_FAILURE);
      return;
    }

    if (this._isCdmaClient(aClientId)) {
      this._switchCallCdma(aClientId, aCallIndex, aCallback);
    } else {
      this._switchCallGsm(aClientId, aCallIndex, aCallback, aRequiredState);
    }
  },

  _switchCallGsm: function(aClientId, aCallIndex, aCallback, aRequiredState) {
    let call = this._currentCalls[aClientId][aCallIndex];
    if (call.state != aRequiredState) {
      aCallback.notifyError(RIL.GECKO_ERROR_GENERIC_FAILURE);
      return;
    }

    this._switchActiveCall(aClientId, aCallback);
  },

  _switchActiveCall: function(aClientId, aCallback) {
    this._sendToRilWorker(aClientId, "switchActiveCall", null,
                          this._defaultCallbackHandler.bind(this, aCallback));
  },

  _switchCallCdma: function(aClientId, aCallIndex, aCallback) {
    let call = this._currentCalls[aClientId][aCallIndex];
    if (!call.isSwitchable) {
      aCallback.notifyError(RIL.GECKO_ERROR_GENERIC_FAILURE);
      return;
    }

    this._sendToRilWorker(aClientId, "cdmaFlash", null,
                          this._defaultCallbackHandler.bind(this, aCallback));
  },

  holdCall: function(aClientId, aCallIndex, aCallback) {
    this._switchCall(aClientId, aCallIndex, aCallback,
                     nsITelephonyService.CALL_STATE_CONNECTED);
  },

  resumeCall: function(aClientId, aCallIndex, aCallback) {
    this._switchCall(aClientId, aCallIndex, aCallback,
                     nsITelephonyService.CALL_STATE_HELD);
  },

  _conferenceCallGsm: function(aClientId, aCallback) {
    this._sendToRilWorker(aClientId, "conferenceCall", null, response => {
      if (!response.success) {
        aCallback.notifyError(RIL.GECKO_ERROR_GENERIC_FAILURE);
        
        this._notifyAllListeners("notifyConferenceError",
                                 ["addError", response.errorMsg]);
        return;
      }

      aCallback.notifySuccess();
    });
  },

  _conferenceCallCdma: function(aClientId, aCallback) {
    for (let index in this._currentCalls[aClientId]) {
      let call = this._currentCalls[aClientId][index];
      if (!call.isMergeable) {
        aCallback.notifyError(RIL.GECKO_ERROR_GENERIC_FAILURE);
        return;
      }
    }

    this._sendToRilWorker(aClientId, "cdmaFlash", null, response => {
      if (!response.success) {
        aCallback.notifyError(RIL.GECKO_ERROR_GENERIC_FAILURE);
        
        this._notifyAllListeners("notifyConferenceError",
                                 ["addError", response.errorMsg]);
        return;
      }

      let calls = [];
      for (let index in this._currentCalls[aClientId]) {
        let call = this._currentCalls[aClientId][index];
        call.state = nsITelephonyService.CALL_STATE_CONNECTED;
        call.isConference = true;
        calls.push(call);
      }
      this._handleCallStateChanged(aClientId, calls);
      this._handleConferenceCallStateChanged(nsITelephonyService.CALL_STATE_CONNECTED);

      aCallback.notifySuccess();
    });
  },

  conferenceCall: function(aClientId, aCallback) {
    if (Object.keys(this._currentCalls[aClientId]).length < 2) {
      aCallback.notifyError(RIL.GECKO_ERROR_GENERIC_FAILURE);
      return;
    }

    if (this._isCdmaClient(aClientId)) {
      this._conferenceCallCdma(aClientId, aCallback);
    } else {
      this._conferenceCallGsm(aClientId, aCallback);
    }
  },

  _separateCallGsm: function(aClientId, aCallIndex, aCallback) {
    this._sendToRilWorker(aClientId, "separateCall", { callIndex: aCallIndex },
                          response => {
      if (!response.success) {
        aCallback.notifyError(RIL.GECKO_ERROR_GENERIC_FAILURE);
        
        this._notifyAllListeners("notifyConferenceError",
                                 ["removeError", response.errorMsg]);
        return;
      }

      aCallback.notifySuccess();
    });
  },

  _removeCdmaSecondCall: function(aClientId) {
    let childCall = this._currentCalls[aClientId][CDMA_SECOND_CALL_INDEX];
    let parentCall = this._currentCalls[aClientId][CDMA_FIRST_CALL_INDEX];

    this._disconnectCalls(aClientId, [childCall]);

    parentCall.isConference = false;
    parentCall.isSwitchable = true;
    parentCall.isMergeable = true;
    this._handleCallStateChanged(aClientId, [childCall, parentCall]);
    this._handleConferenceCallStateChanged(nsITelephonyService.CALL_STATE_UNKNOWN);
  },

  
  
  
  _separateCallCdma: function(aClientId, aCallIndex, aCallback) {
    this._sendToRilWorker(aClientId, "cdmaFlash", null, response => {
      if (!response.success) {
        aCallback.notifyError(RIL.GECKO_ERROR_GENERIC_FAILURE);
        
        this._notifyAllListeners("notifyConferenceError",
                                 ["removeError", response.errorMsg]);
        return;
      }

      this._removeCdmaSecondCall(aClientId);
      aCallback.notifySuccess();
    });
  },

  separateCall: function(aClientId, aCallIndex, aCallback) {
    let call = this._currentCalls[aClientId][aCallIndex];
    if (!call || !call.isConference) {
      aCallback.notifyError(RIL.GECKO_ERROR_GENERIC_FAILURE);
      return;
    }

    if (this._isCdmaClient(aClientId)) {
      this._separateCallCdma(aClientId, aCallIndex, aCallback);
    } else {
      this._separateCallGsm(aClientId, aCallIndex, aCallback);
    }
  },

  hangUpConference: function(aClientId, aCallback) {
    
    if (this._isCdmaClient(aClientId)) {
      this._sendToRilWorker(aClientId, "hangUpCall",
                            { callIndex: CDMA_FIRST_CALL_INDEX },
                            this._defaultCallbackHandler.bind(this, aCallback));
      return;
    }

    let foreground = this._currentConferenceState == nsITelephonyService.CALL_STATE_CONNECTED;
    this._sendToRilWorker(aClientId,
                          foreground ? "hangUpForeground" : "hangUpBackground",
                          null,
                          this._defaultCallbackHandler.bind(this, aCallback));
  },

  _switchConference: function(aClientId, aCallback) {
    
    if (this._isCdmaClient(aClientId)) {
      aCallback.notifyError(RIL.GECKO_ERROR_GENERIC_FAILURE);
      return;
    }

    this._switchActiveCall(aClientId, aCallback);
  },

  holdConference: function(aClientId, aCallback) {
    this._switchConference(aClientId, aCallback);
  },

  resumeConference: function(aClientId, aCallback) {
    this._switchConference(aClientId, aCallback);
  },

  sendUSSD: function(aClientId, aUssd, aCallback) {
    this._sendToRilWorker(aClientId, "sendUSSD", { ussd: aUssd },
                          this._defaultCallbackHandler.bind(this, aCallback));
  },

  cancelUSSD: function(aClientId, aCallback) {
    this._sendToRilWorker(aClientId, "cancelUSSD", {},
                          this._defaultCallbackHandler.bind(this, aCallback));
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

  



  _notifyCallEnded: function(aCall) {
    let duration = ("started" in aCall && typeof aCall.started == "number") ?
      new Date().getTime() - aCall.started : 0;

    gTelephonyMessenger.notifyCallEnded(aCall.clientId,
                                        aCall.number,
                                        this._cdmaCallWaitingNumber,
                                        aCall.isEmergency,
                                        duration,
                                        aCall.isOutgoing,
                                        aCall.hangUpLocal);

    
    this._cdmaCallWaitingNumber = null;
  },

  









  _disconnectCalls: function(aClientId, aCalls,
                             aFailCause = RIL.GECKO_CALL_ERROR_NORMAL_CALL_CLEARING) {
    if (DEBUG) debug("_disconnectCalls: " + JSON.stringify(aCalls));

    
    
    let disconnectedCalls = aCalls.slice();

    for (let call in aCalls) {
      while (call.childId) {
        call = this._currentCalls[aClientId][call.childId];
        disconnectedCalls.push(call);
      }
    }

    
    disconnectedCalls = [...Set(disconnectedCalls)];

    let callsForStateChanged = [];

    disconnectedCalls.forEach(call => {
      call.state = nsITelephonyService.CALL_STATE_DISCONNECTED;
      call.failCause = aFailCause;

      if (call.parentId) {
        let parentCall = this._currentCalls[aClientId][call.parentId];
        delete parentCall.childId;
      }

      this._notifyCallEnded(call);

      if (call.hangUpLocal || !call.failCause ||
          call.failCause === RIL.GECKO_CALL_ERROR_NORMAL_CALL_CLEARING) {
        callsForStateChanged.push(call);
      } else {
        this._notifyAllListeners("notifyError",
                                 [aClientId, call.callIndex, call.failCause]);
      }

      delete this._currentCalls[aClientId][call.callIndex];
    });

    return callsForStateChanged;
  },

  





  notifyCallRing: function() {
    
    
    this._acquireCallRingWakeLock();

    gTelephonyMessenger.notifyNewCall();
  },

  






  notifyCurrentCalls: function(aClientId, aCalls) {
    
    let hasRemovedCalls = () => {
      let newIndexes = new Set(Object.keys(aCalls));
      for (let i in this._currentCalls[aClientId]) {
        if (!newIndexes.has(i)) {
          return true;
        }
      }
      return false;
    };

    
    if (!hasRemovedCalls()) {
      this._handleCurrentCalls(aClientId, aCalls);
    } else {
      this._sendToRilWorker(aClientId, "getFailCause", null, response => {
        this._handleCurrentCalls(aClientId, aCalls, response.failCause);
      });
    }
  },

  _handleCurrentCalls: function(aClientId, aCalls,
                                aFailCause = RIL.GECKO_CALL_ERROR_NORMAL_CALL_CLEARING) {
    if (DEBUG) debug("handleCurrentCalls: " + JSON.stringify(aCalls) +
                     ", failCause: " + aFailCause);

    let changedCalls = new Set();
    let removedCalls = new Set();

    let allIndexes = new Set([...Object.keys(this._currentCalls[aClientId]),
                              ...Object.keys(aCalls)]);

    for (let i of allIndexes) {
      let call = this._currentCalls[aClientId][i];
      let rilCall = aCalls[i];

      
      if (call && !rilCall) {  
        removedCalls.add(call);
      } else if (call && rilCall) {  
        if (this._updateCallFromRil(call, rilCall)) {
          changedCalls.add(call);
        }
      } else {  
        this._currentCalls[aClientId][i] = call = new Call(aClientId, i);
        this._updateCallFromRil(call, rilCall);
        changedCalls.add(call);

        
        if (this._ongoingDial && this._ongoingDial.clientId === aClientId &&
            call.state !== nsITelephonyService.CALL_STATE_INCOMING) {
          this._ongoingDial.callback.notifyDialCallSuccess(aClientId, i,
                                                           call.number);
          this._ongoingDial = null;
        }
      }
    }

    
    
    let disconnectedCalls = this._disconnectCalls(aClientId, [...removedCalls], aFailCause);
    disconnectedCalls.forEach(call => changedCalls.add(call));

    
    let [newConferenceState, conferenceChangedCalls] = this._updateConference(aClientId);
    conferenceChangedCalls.forEach(call => changedCalls.add(call));

    this._handleCallStateChanged(aClientId, [...changedCalls]);

    
    
    if (newConferenceState != this._currentConferenceState) {
      this._handleConferenceCallStateChanged(newConferenceState);
    }

    this._updateAudioState(aClientId);

    
    if (this._cachedDialRequest && !this._isActive(aClientId)) {
      if (DEBUG) debug("All calls held. Perform the cached dial request.");

      let request = this._cachedDialRequest;
      this._sendDialCallRequest(request.clientId, request.options,
                                request.callback);
      this._cachedDialRequest = null;
    }
  },

  


  _handleCallStateChanged: function(aClientId, aCalls) {
    if (DEBUG) debug("handleCallStateChanged: " + JSON.stringify(aCalls));

    if (aCalls.some(call => call.state == nsITelephonyService.CALL_STATE_DIALING)) {
      gTelephonyMessenger.notifyNewCall();
    }

    let allInfo = aCalls.map(call => new TelephonyCallInfo(call));
    this._notifyAllListeners("callStateChanged", [allInfo.length, allInfo]);
  },

  notifyCdmaCallWaiting: function(aClientId, aCall) {
    
    
    this._acquireCallRingWakeLock();

    let call = this._currentCalls[aClientId][CDMA_SECOND_CALL_INDEX];
    if (call) {
      
      
      this._removeCdmaSecondCall(aClientId);
    }

    this._cdmaCallWaitingNumber = aCall.number;

    this._notifyAllListeners("notifyCdmaCallWaiting", [aClientId,
                                                       aCall.number,
                                                       aCall.numberPresentation,
                                                       aCall.name,
                                                       aCall.namePresentation]);
  },

  notifySupplementaryService: function(aClientId, aNumber, aNotification) {
    let notification = this._convertRILSuppSvcNotification(aNotification);

    
    let callIndex = -1;

    let indexes = Object.keys(this.currentCalls);
    if (indexes.length === 1) {
      
      callIndex = indexes[0];
    } else {
      
      if (aNumber) {
        for (let i in this._currentCalls) {
          let call = this._currentCalls[aClientId][i];
          if (call.number === aNumber) {
            callIndex = i;
            break;
          }
        }
      }
    }

    this._notifyAllListeners("supplementaryServiceNotification",
                             [aClientId, callIndex, notification]);
  },

  _handleConferenceCallStateChanged: function(aState) {
    if (DEBUG) debug("handleConferenceCallStateChanged: " + aState);
    this._currentConferenceState = aState;
    this._notifyAllListeners("conferenceCallStateChanged", [aState]);
  },

  notifyUssdReceived: function(aClientId, aMessage, aSessionEnded) {
    if (DEBUG) {
      debug("notifyUssdReceived for " + aClientId + ": " +
            aMessage + " (sessionEnded : " + aSessionEnded + ")");
    }

    gTelephonyMessenger.notifyUssdReceived(aClientId, aMessage, aSessionEnded);
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
