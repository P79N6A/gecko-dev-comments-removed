




"use strict";

const {classes: Cc, interfaces: Ci, utils: Cu, results: Cr} = Components;

Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://gre/modules/Services.jsm");
Cu.import("resource://gre/modules/Promise.jsm");

var RIL = {};
Cu.import("resource://gre/modules/ril_consts.js", RIL);

const GONK_TELEPHONYPROVIDER_CONTRACTID =
  "@mozilla.org/telephony/gonktelephonyprovider;1";
const GONK_TELEPHONYPROVIDER_CID =
  Components.ID("{67d26434-d063-4d28-9f48-5b3189788155}");

const NS_XPCOM_SHUTDOWN_OBSERVER_ID   = "xpcom-shutdown";

const NS_PREFBRANCH_PREFCHANGE_TOPIC_ID = "nsPref:changed";

const kPrefRilNumRadioInterfaces = "ril.numRadioInterfaces";
const kPrefRilDebuggingEnabled = "ril.debugging.enabled";
const kPrefDefaultServiceId = "dom.telephony.defaultServiceId";

const nsIAudioManager = Ci.nsIAudioManager;
const nsITelephonyProvider = Ci.nsITelephonyProvider;

const CALL_WAKELOCK_TIMEOUT = 5000;


const CDMA_SECOND_CALL_INDEX = 2;

const DIAL_ERROR_INVALID_STATE_ERROR = "InvalidStateError";
const DIAL_ERROR_OTHER_CONNECTION_IN_USE = "OtherConnectionInUse";


const OUTGOING_PLACEHOLDER_CALL_INDEX = 0xffffffff;

let DEBUG;
function debug(s) {
  dump("TelephonyProvider: " + s + "\n");
}

XPCOMUtils.defineLazyGetter(this, "gAudioManager", function getAudioManager() {
  try {
    return Cc["@mozilla.org/telephony/audiomanager;1"]
             .getService(nsIAudioManager);
  } catch (ex) {
    

    
    
    if (DEBUG) debug("Using fake audio manager.");
    return {
      microphoneMuted: false,
      masterVolume: 1.0,
      masterMuted: false,
      phoneState: nsIAudioManager.PHONE_STATE_CURRENT,
      _forceForUse: {},

      setForceForUse: function(usage, force) {
        this._forceForUse[usage] = force;
      },

      getForceForUse: function(usage) {
        return this._forceForUse[usage] || nsIAudioManager.FORCE_NONE;
      }
    };
  }
});

XPCOMUtils.defineLazyServiceGetter(this, "gRadioInterfaceLayer",
                                   "@mozilla.org/ril;1",
                                   "nsIRadioInterfaceLayer");

XPCOMUtils.defineLazyServiceGetter(this, "gPowerManagerService",
                                   "@mozilla.org/power/powermanagerservice;1",
                                   "nsIPowerManagerService");

XPCOMUtils.defineLazyServiceGetter(this, "gSystemMessenger",
                                   "@mozilla.org/system-message-internal;1",
                                   "nsISystemMessagesInternal");

XPCOMUtils.defineLazyGetter(this, "gPhoneNumberUtils", function() {
  let ns = {};
  Cu.import("resource://gre/modules/PhoneNumberUtils.jsm", ns);
  return ns.PhoneNumberUtils;
});

function SingleCall(options){
  this.clientId = options.clientId;
  this.callIndex = options.callIndex;
  this.state = options.state;
  this.number = options.number;
  this.isOutgoing = options.isOutgoing;
  this.isEmergency = options.isEmergency;
  this.isConference = options.isConference;
}
SingleCall.prototype = {
  clientId: null,
  callIndex: null,
  state: null,
  number: null,
  isOutgoing: false,
  isEmergency: false,
  isConference: false
};

function ConferenceCall(state){
  this.state = state;
}
ConferenceCall.prototype = {
  state: null
};

function TelephonyProvider() {
  this._numClients = gRadioInterfaceLayer.numRadioInterfaces;
  this._listeners = [];
  this._currentCalls = {};
  this._updateDebugFlag();
  this.defaultServiceId = this._getDefaultServiceId();

  Services.prefs.addObserver(kPrefRilDebuggingEnabled, this, false);
  Services.prefs.addObserver(kPrefDefaultServiceId, this, false);

  Services.obs.addObserver(this, NS_XPCOM_SHUTDOWN_OBSERVER_ID, false);

  for (let i = 0; i < this._numClients; ++i) {
    this._enumerateCallsForClient(i);
  }
}
TelephonyProvider.prototype = {
  classID: GONK_TELEPHONYPROVIDER_CID,
  classInfo: XPCOMUtils.generateCI({classID: GONK_TELEPHONYPROVIDER_CID,
                                    contractID: GONK_TELEPHONYPROVIDER_CONTRACTID,
                                    classDescription: "TelephonyProvider",
                                    interfaces: [Ci.nsITelephonyProvider,
                                                 Ci.nsIGonkTelephonyProvider],
                                    flags: Ci.nsIClassInfo.SINGLETON}),
  QueryInterface: XPCOMUtils.generateQI([Ci.nsITelephonyProvider,
                                         Ci.nsIGonkTelephonyProvider,
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

  _matchActiveSingleCall: function(aCall) {
    return this._activeCall &&
           this._activeCall instanceof SingleCall &&
           this._activeCall.clientId === aCall.clientId &&
           this._activeCall.callIndex === aCall.callIndex;
  },

  


  _activeCall: null,
  _updateCallAudioState: function(aCall, aConferenceState) {
    if (aConferenceState === nsITelephonyProvider.CALL_STATE_CONNECTED) {
      this._activeCall = new ConferenceCall(aConferenceState);
      gAudioManager.phoneState = nsIAudioManager.PHONE_STATE_IN_CALL;
      if (this.speakerEnabled) {
        gAudioManager.setForceForUse(nsIAudioManager.USE_COMMUNICATION,
                                     nsIAudioManager.FORCE_SPEAKER);
      }
      if (DEBUG) {
        debug("Active call, put audio system into PHONE_STATE_IN_CALL: " +
              gAudioManager.phoneState);
      }
      return;
    }

    if (aConferenceState === nsITelephonyProvider.CALL_STATE_UNKNOWN ||
        aConferenceState === nsITelephonyProvider.CALL_STATE_HELD) {
      if (this._activeCall instanceof ConferenceCall) {
        this._activeCall = null;
        gAudioManager.phoneState = nsIAudioManager.PHONE_STATE_NORMAL;
        if (DEBUG) {
          debug("No active call, put audio system into PHONE_STATE_NORMAL: " +
                gAudioManager.phoneState);
        }
      }
      return;
    }

    if (!aCall) {
      return;
    }

    if (aCall.isConference) {
      if (this._matchActiveSingleCall(aCall)) {
        this._activeCall = null;
      }
      return;
    }

    switch (aCall.state) {
      case nsITelephonyProvider.CALL_STATE_DIALING: 
      case nsITelephonyProvider.CALL_STATE_ALERTING:
      case nsITelephonyProvider.CALL_STATE_CONNECTED:
        aCall.isActive = true;
        this._activeCall = new SingleCall(aCall);
        gAudioManager.phoneState = nsIAudioManager.PHONE_STATE_IN_CALL;
        if (this.speakerEnabled) {
          gAudioManager.setForceForUse(nsIAudioManager.USE_COMMUNICATION,
                                       nsIAudioManager.FORCE_SPEAKER);
        }
        if (DEBUG) {
          debug("Active call, put audio system into PHONE_STATE_IN_CALL: " +
                gAudioManager.phoneState);
        }
        break;

      case nsITelephonyProvider.CALL_STATE_INCOMING:
        aCall.isActive = false;
        if (!this._activeCall) {
          
          
          gAudioManager.phoneState = nsIAudioManager.PHONE_STATE_RINGTONE;
          if (DEBUG) {
            debug("Incoming call, put audio system into PHONE_STATE_RINGTONE: " +
                  gAudioManager.phoneState);
          }
        }
        break;

      case nsITelephonyProvider.CALL_STATE_HELD: 
      case nsITelephonyProvider.CALL_STATE_DISCONNECTED:
        aCall.isActive = false;
        if (this._matchActiveSingleCall(aCall)) {
          
          this._activeCall = null;
        }

        if (!this._activeCall) {
          
          gAudioManager.phoneState = nsIAudioManager.PHONE_STATE_NORMAL;
          if (DEBUG) {
            debug("No active call, put audio system into PHONE_STATE_NORMAL: " +
                  gAudioManager.phoneState);
          }
        }
        break;
    }
  },

  _convertRILCallState: function(aState) {
    switch (aState) {
      case RIL.CALL_STATE_UNKNOWN:
        return nsITelephonyProvider.CALL_STATE_UNKNOWN;
      case RIL.CALL_STATE_ACTIVE:
        return nsITelephonyProvider.CALL_STATE_CONNECTED;
      case RIL.CALL_STATE_HOLDING:
        return nsITelephonyProvider.CALL_STATE_HELD;
      case RIL.CALL_STATE_DIALING:
        return nsITelephonyProvider.CALL_STATE_DIALING;
      case RIL.CALL_STATE_ALERTING:
        return nsITelephonyProvider.CALL_STATE_ALERTING;
      case RIL.CALL_STATE_INCOMING:
      case RIL.CALL_STATE_WAITING:
        return nsITelephonyProvider.CALL_STATE_INCOMING;
      default:
        throw new Error("Unknown rilCallState: " + aState);
    }
  },

  _convertRILSuppSvcNotification: function(aNotification) {
    switch (aNotification) {
      case RIL.GECKO_SUPP_SVC_NOTIFICATION_REMOTE_HELD:
        return nsITelephonyProvider.NOTIFICATION_REMOTE_HELD;
      case RIL.GECKO_SUPP_SVC_NOTIFICATION_REMOTE_RESUMED:
        return nsITelephonyProvider.NOTIFICATION_REMOTE_RESUMED;
      default:
        if (DEBUG) {
          debug("Unknown rilSuppSvcNotification: " + aNotification);
        }
        return;
    }
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

    this._getClient(aClientId).sendWorkerMessage("enumerateCalls", null,
                                                 (function(response) {
      if (!this._currentCalls[aClientId]) {
        this._currentCalls[aClientId] = {};
      }
      for (let call of response.calls) {
        call.clientId = aClientId;
        call.state = this._convertRILCallState(call.state);
        call.isActive = this._matchActiveSingleCall(call);
        call.isSwitchable = true;
        call.isMergeable = true;

        this._currentCalls[aClientId][call.callIndex] = call;
      }

      return false;
    }).bind(this));
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
                                     call.isActive, call.isOutgoing,
                                     call.isEmergency, call.isConference,
                                     call.isSwitchable, call.isMergeable);
      }
    }
    aListener.enumerateCallStateComplete();
  },

  isDialing: false,
  dial: function(aClientId, aNumber, aIsEmergency, aTelephonyCallback) {
    if (DEBUG) debug("Dialing " + (aIsEmergency ? "emergency " : "") + aNumber);

    if (this.isDialing) {
      if (DEBUG) debug("Already has a dialing call. Drop.");
      aTelephonyCallback.notifyDialError(DIAL_ERROR_INVALID_STATE_ERROR);
      return;
    }

    function hasCallsOnOtherClient(aClientId) {
      for (let cid = 0; cid < this._numClients; ++cid) {
        if (cid === aClientId) {
          continue;
        }
        if (Object.keys(this._currentCalls[cid]).length !== 0) {
          return true;
        }
      }
      return false;
    }

    
    
    if (hasCallsOnOtherClient.call(this, aClientId)) {
      if (DEBUG) debug("Already has a call on other sim. Drop.");
      aTelephonyCallback.notifyDialError(DIAL_ERROR_OTHER_CONNECTION_IN_USE);
      return;
    }

    
    function numCallsOnLine(aClientId) {
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
    }

    if (numCallsOnLine.call(this, aClientId) >= 2) {
      if (DEBUG) debug("Has more than 2 calls on line. Drop.");
      aTelephonyCallback.notifyDialError(DIAL_ERROR_INVALID_STATE_ERROR);
      return;
    }

    
    
    if (!aIsEmergency) {
      aNumber = gPhoneNumberUtils.normalize(aNumber);
    }

    
    if (!gPhoneNumberUtils.isPlainPhoneNumber(aNumber)) {
      
      if (DEBUG) debug("Number '" + aNumber + "' is not viable. Drop.");
      let errorMsg = RIL.RIL_CALL_FAILCAUSE_TO_GECKO_CALL_ERROR[RIL.CALL_FAIL_UNOBTAINABLE_NUMBER];
      aTelephonyCallback.notifyDialError(errorMsg);
      return;
    }

    function onCdmaDialSuccess() {
      let indexes = Object.keys(this._currentCalls[aClientId]);
      if (indexes.length != 1 ) {
        aTelephonyCallback.notifyDialSuccess();
        return;
      }

      
      let childCall = {
        callIndex: CDMA_SECOND_CALL_INDEX,
        state: RIL.CALL_STATE_DIALING,
        number: aNumber,
        isOutgoing: true,
        isEmergency: false,
        isConference: false,
        isSwitchable: false,
        isMergeable: true,
        parentId: indexes[0]
      };
      aTelephonyCallback.notifyDialSuccess();

      
      this.notifyCallStateChanged(aClientId, childCall);

      childCall.state = RIL.CALL_STATE_ACTIVE;
      this.notifyCallStateChanged(aClientId, childCall);

      let parentCall = this._currentCalls[aClientId][childCall.parentId];
      parentCall.childId = CDMA_SECOND_CALL_INDEX;
      parentCall.state = RIL.CALL_STATE_HOLDING;
      parentCall.isSwitchable = false;
      parentCall.isMergeable = true;
      this.notifyCallStateChanged(aClientId, parentCall);
    };

    this.isDialing = true;
    this._getClient(aClientId).sendWorkerMessage("dial", {
      number: aNumber,
      isDialEmergency: aIsEmergency
    }, (function(response) {
      this.isDialing = false;
      if (!response.success) {
        aTelephonyCallback.notifyDialError(response.errorMsg);
        return false;
      }

      if (response.isCdma) {
        onCdmaDialSuccess.call(this);
      } else {
        aTelephonyCallback.notifyDialSuccess();
      }
      return false;
    }).bind(this));
  },

  hangUp: function(aClientId, aCallIndex) {
    let parentId = this._currentCalls[aClientId][aCallIndex].parentId;
    if (parentId) {
      
      
      this.hangUp(aClientId, parentId);
    } else {
      this._getClient(aClientId).sendWorkerMessage("hangUp", { callIndex: aCallIndex });
    }
  },

  startTone: function(aClientId, aDtmfChar) {
    this._getClient(aClientId).sendWorkerMessage("startTone", { dtmfChar: aDtmfChar });
  },

  stopTone: function(aClientId) {
    this._getClient(aClientId).sendWorkerMessage("stopTone");
  },

  answerCall: function(aClientId, aCallIndex) {
    this._getClient(aClientId).sendWorkerMessage("answerCall", { callIndex: aCallIndex });
  },

  rejectCall: function(aClientId, aCallIndex) {
    this._getClient(aClientId).sendWorkerMessage("rejectCall", { callIndex: aCallIndex });
  },

  holdCall: function(aClientId, aCallIndex) {
    let call = this._currentCalls[aClientId][aCallIndex];
    if (!call || !call.isSwitchable) {
      
      
      return;
    }

    this._getClient(aClientId).sendWorkerMessage("holdCall", { callIndex: aCallIndex });
  },

  resumeCall: function(aClientId, aCallIndex) {
    let call = this._currentCalls[aClientId][aCallIndex];
    if (!call || !call.isSwitchable) {
      
      
      return;
    }

    this._getClient(aClientId).sendWorkerMessage("resumeCall", { callIndex: aCallIndex });
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
    };

    this._getClient(aClientId).sendWorkerMessage("conferenceCall", null,
                                                 (function(response) {
      if (!response.success) {
        this._notifyAllListeners("notifyConferenceError", [response.errorName,
                                                           response.errorMsg]);
        return false;
      }

      if (response.isCdma) {
        onCdmaConferenceCallSuccess.call(this);
      }
      return false;
    }).bind(this));
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
    };

    this._getClient(aClientId).sendWorkerMessage("separateCall", {
      callIndex: aCallIndex
    }, (function(response) {
      if (!response.success) {
        this._notifyAllListeners("notifyConferenceError", [response.errorName,
                                                           response.errorMsg]);
        return false;
      }

      if (response.isCdma) {
        onCdmaSeparateCallSuccess.call(this);
      }
      return false;
    }).bind(this));
  },

  holdConference: function(aClientId) {
    this._getClient(aClientId).sendWorkerMessage("holdConference");
  },

  resumeConference: function(aClientId) {
    this._getClient(aClientId).sendWorkerMessage("resumeConference");
  },

  get microphoneMuted() {
    return gAudioManager.microphoneMuted;
  },

  set microphoneMuted(aMuted) {
    if (aMuted == this.microphoneMuted) {
      return;
    }
    gAudioManager.microphoneMuted = aMuted;

    if (!this._activeCall) {
      gAudioManager.phoneState = nsIAudioManager.PHONE_STATE_NORMAL;
    }
  },

  get speakerEnabled() {
    let force = gAudioManager.getForceForUse(nsIAudioManager.USE_COMMUNICATION);
    return (force == nsIAudioManager.FORCE_SPEAKER);
  },

  set speakerEnabled(aEnabled) {
    if (aEnabled == this.speakerEnabled) {
      return;
    }
    let force = aEnabled ? nsIAudioManager.FORCE_SPEAKER :
                           nsIAudioManager.FORCE_NONE;
    gAudioManager.setForceForUse(nsIAudioManager.USE_COMMUNICATION, force);

    if (!this._activeCall) {
      gAudioManager.phoneState = nsIAudioManager.PHONE_STATE_NORMAL;
    }
  },

  



  


  notifyCallDisconnected: function(aClientId, aCall) {
    if (DEBUG) debug("handleCallDisconnected: " + JSON.stringify(aCall));

    aCall.state = nsITelephonyProvider.CALL_STATE_DISCONNECTED;
    let duration = ("started" in aCall && typeof aCall.started == "number") ?
      new Date().getTime() - aCall.started : 0;
    let data = {
      number: aCall.number,
      serviceId: aClientId,
      emergency: aCall.isEmergency,
      duration: duration,
      direction: aCall.isOutgoing ? "outgoing" : "incoming"
    };
    gSystemMessenger.broadcastMessage("telephony-call-ended", data);

    aCall.clientId = aClientId;
    this._updateCallAudioState(aCall, null);

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

    if (!aCall.failCause ||
        aCall.failCause === RIL.GECKO_CALL_ERROR_NORMAL_CALL_CLEARING) {
      this._notifyAllListeners("callStateChanged", [aClientId,
                                                    aCall.callIndex,
                                                    aCall.state,
                                                    aCall.number,
                                                    aCall.isActive,
                                                    aCall.isOutgoing,
                                                    aCall.isEmergency,
                                                    aCall.isConference,
                                                    aCall.isSwitchable,
                                                    aCall.isMergeable]);
    } else {
      this.notifyCallError(aClientId, aCall.callIndex, aCall.failCause);
    }
    delete this._currentCalls[aClientId][aCall.callIndex];

    if (manualConfStateChange) {
      this.notifyConferenceCallStateChanged(RIL.CALL_STATE_UNKNOWN);
    }
  },

  


  notifyCallError: function(aClientId, aCallIndex, aErrorMsg) {
    this._notifyAllListeners("notifyError", [aClientId, aCallIndex, aErrorMsg]);
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

    if (aCall.state == nsITelephonyProvider.CALL_STATE_DIALING) {
      gSystemMessenger.broadcastMessage("telephony-new-call", {});
    }

    aCall.clientId = aClientId;
    this._updateCallAudioState(aCall, null);

    let call = this._currentCalls[aClientId][aCall.callIndex];
    if (call) {
      call.state = aCall.state;
      call.isConference = aCall.isConference;
      call.isEmergency = aCall.isEmergency;
      call.isActive = aCall.isActive;
      call.isSwitchable = aCall.isSwitchable != null ?
                          aCall.isSwitchable : call.isSwitchable;
      call.isMergeable = aCall.isMergeable != null ?
                         aCall.isMergeable : call.isMergeable;
    } else {
      call = aCall;
      call.isSwitchable = aCall.isSwitchable != null ?
                          aCall.isSwitchable : true;
      call.isMergeable = aCall.isMergeable != null ?
                         aCall.isMergeable : true;

      
      if (this._currentCalls[aClientId][OUTGOING_PLACEHOLDER_CALL_INDEX] &&
          call.callIndex != OUTGOING_PLACEHOLDER_CALL_INDEX &&
          call.isOutgoing) {
        delete this._currentCalls[aClientId][OUTGOING_PLACEHOLDER_CALL_INDEX];
      }

      this._currentCalls[aClientId][aCall.callIndex] = call;
    }

    this._notifyAllListeners("callStateChanged", [aClientId,
                                                  call.callIndex,
                                                  call.state,
                                                  call.number,
                                                  call.isActive,
                                                  call.isOutgoing,
                                                  call.isEmergency,
                                                  call.isConference,
                                                  call.isSwitchable,
                                                  call.isMergeable]);
  },

  notifyCdmaCallWaiting: function(aClientId, aNumber) {
    
    
    this._acquireCallRingWakeLock();

    let call = this._currentCalls[aClientId][CDMA_SECOND_CALL_INDEX];
    if (call) {
      
      
      this.notifyCallDisconnected(aClientId, call);
    }
    this._notifyAllListeners("notifyCdmaCallWaiting", [aClientId, aNumber]);
  },

  notifySupplementaryService: function(aClientId, aCallIndex, aNotification) {
    let notification = this._convertRILSuppSvcNotification(aNotification);
    this._notifyAllListeners("supplementaryServiceNotification",
                             [aClientId, aCallIndex, notification]);
  },

  notifyConferenceCallStateChanged: function(aState) {
    if (DEBUG) debug("handleConferenceCallStateChanged: " + aState);
    aState = this._convertRILCallState(aState);
    this._updateCallAudioState(null, aState);

    this._notifyAllListeners("conferenceCallStateChanged", [aState]);
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

this.NSGetFactory = XPCOMUtils.generateNSGetFactory([TelephonyProvider]);
