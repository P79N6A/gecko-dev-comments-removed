




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

      setForceForUse: function setForceForUse(usage, force) {
        this._forceForUse[usage] = force;
      },

      getForceForUse: function setForceForUse(usage) {
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

XPCOMUtils.defineLazyGetter(this, "gPhoneNumberUtils", function () {
  let ns = {};
  Cu.import("resource://gre/modules/PhoneNumberUtils.jsm", ns);
  return ns.PhoneNumberUtils;
});

function TelephonyProvider() {
  this._numClients = gRadioInterfaceLayer.numRadioInterfaces;
  this._listeners = [];
  this._updateDebugFlag();
  this.defaultServiceId = this._getDefaultServiceId();

  Services.prefs.addObserver(kPrefRilDebuggingEnabled, this, false);
  Services.prefs.addObserver(kPrefDefaultServiceId, this, false);

  Services.obs.addObserver(this, NS_XPCOM_SHUTDOWN_OBSERVER_ID, false);
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

  _acquireCallRingWakeLock: function _acquireCallRingWakeLock() {
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

  _releaseCallRingWakeLock: function _releaseCallRingWakeLock() {
    if (DEBUG) debug("Releasing the CPU wake lock for handling incoming call.");
    if (this._callRingWakeLockTimer) {
      this._callRingWakeLockTimer.cancel();
    }
    if (this._callRingWakeLock) {
      this._callRingWakeLock.unlock();
      this._callRingWakeLock = null;
    }
  },

  _getClient: function _getClient(aClientId) {
    return gRadioInterfaceLayer.getRadioInterface(aClientId);
  },

  
  _listeners: null,
  _notifyAllListeners: function _notifyAllListeners(aMethodName, aArgs) {
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

  _matchActiveCall: function _matchActiveCall(aCall) {
    if (this._activeCall &&
        this._activeCall.callIndex == aCall.callIndex &&
        this._activeCall.clientId == aCall.clientId) {
      return true;
    }

    return false;
  },

  


  _activeCall: null,
  _updateCallAudioState: function _updateCallAudioState(aCall,
                                                        aConferenceState) {
    if (aConferenceState === nsITelephonyProvider.CALL_STATE_CONNECTED) {
      gAudioManager.phoneState = nsIAudioManager.PHONE_STATE_IN_CALL;
      if (this.speakerEnabled) {
        gAudioManager.setForceForUse(nsIAudioManager.USE_COMMUNICATION,
                                     nsIAudioManager.FORCE_SPEAKER);
      }
      return;
    }
    if (aConferenceState === nsITelephonyProvider.CALL_STATE_UNKNOWN ||
        aConferenceState === nsITelephonyProvider.CALL_STATE_HELD) {
      if (!this._activeCall) {
        gAudioManager.phoneState = nsIAudioManager.PHONE_STATE_NORMAL;
      }
      return;
    }

    if (!aCall) {
      return;
    }

    if (aCall.isConference) {
      if (this._matchActiveCall(aCall)) {
        this._activeCall = null;
      }
      return;
    }

    switch (aCall.state) {
      case nsITelephonyProvider.CALL_STATE_DIALING: 
      case nsITelephonyProvider.CALL_STATE_ALERTING:
      case nsITelephonyProvider.CALL_STATE_CONNECTED:
        aCall.isActive = true;
        this._activeCall = aCall;
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
        if (this._matchActiveCall(aCall)) {
          
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

  _convertRILCallState: function _convertRILCallState(aState) {
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

  _convertRILSuppSvcNotification: function _convertRILSuppSvcNotification(aNotification) {
    switch (aNotification) {
      case RIL.GECKO_SUPP_SVC_NOTIFICATION_REMOTE_HELD:
        return nsITelephonyProvider.NOTIFICATION_REMOTE_HELD;
      case RIL.GECKO_SUPP_SVC_NOTIFICATION_REMOTE_RESUMED:
        return nsITelephonyProvider.NOTIFICATION_REMOTE_RESUMED;
      default:
        throw new Error("Unknown rilSuppSvcNotification: " + aNotification);
    }
  },

  _validateNumber: function _validateNumber(aNumber) {
    
    if (gPhoneNumberUtils.isPlainPhoneNumber(aNumber)) {
      return true;
    }

    let errorMsg = RIL.RIL_CALL_FAILCAUSE_TO_GECKO_CALL_ERROR[RIL.CALL_FAIL_UNOBTAINABLE_NUMBER];
    let currentThread = Services.tm.currentThread;
    currentThread.dispatch(this.notifyCallError.bind(this, -1, errorMsg),
                           Ci.nsIThread.DISPATCH_NORMAL);
    if (DEBUG) {
      debug("Number '" + aNumber + "' doesn't seem to be a viable number. Drop.");
    }

    return false;
  },

  _updateDebugFlag: function _updateDebugFlag() {
    try {
      DEBUG = RIL.DEBUG_RIL ||
              Services.prefs.getBoolPref(kPrefRilDebuggingEnabled);
    } catch (e) {}
  },

  _getDefaultServiceId: function _getDefaultServiceId() {
    let id = Services.prefs.getIntPref(kPrefDefaultServiceId);
    let numRil = Services.prefs.getIntPref(kPrefRilNumRadioInterfaces);

    if (id >= numRil || id < 0) {
      id = 0;
    }

    return id;
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

  _enumerateCallsForClient: function _enumerateCallsForClient(aClientId,
                                                              aListener) {
    if (DEBUG) debug("Enumeration of calls for client " + aClientId);

    let deferred = Promise.defer();

    this._getClient(aClientId).sendWorkerMessage("enumerateCalls", null,
                                                 (function(response) {
      for (let call of response.calls) {
        call.clienId = aClientId;
        call.state = this._convertRILCallState(call.state);
        call.isActive = this._matchActiveCall(call);

        aListener.enumerateCallState(call.clientId, call.callIndex,
                                     call.state, call.number,
                                     call.isActive, call.isOutgoing,
                                     call.isEmergency, call.isConference);
      }
      deferred.resolve();

      return false;
    }).bind(this));

    return deferred.promise;
  },

  enumerateCalls: function(aListener) {
    if (DEBUG) debug("Requesting enumeration of calls for callback");

    let promise = Promise.resolve();
    for (let i = 0; i < this._numClients; ++i) {
      promise = promise.then(this._enumerateCallsForClient.bind(this, i, aListener));
    }
    promise.then(function() {
      aListener.enumerateCallStateComplete();
    });
  },

  dial: function(aClientId, aNumber, aIsEmergency) {
    if (DEBUG) debug("Dialing " + (aIsEmergency ? "emergency " : "") + aNumber);
    
    
    if (!aIsEmergency) {
      aNumber = gPhoneNumberUtils.normalize(aNumber);
    }
    if (this._validateNumber(aNumber)) {
      this._getClient(aClientId).sendWorkerMessage("dial", {
        number: aNumber,
        isDialEmergency: aIsEmergency
      });
    }
  },

  hangUp: function(aClientId, aCallIndex) {
    this._getClient(aClientId).sendWorkerMessage("hangUp", { callIndex: aCallIndex });
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
    this._getClient(aClientId).sendWorkerMessage("holdCall", { callIndex: aCallIndex });
  },

  resumeCall: function(aClientId, aCallIndex) {
    this._getClient(aClientId).sendWorkerMessage("resumeCall", { callIndex: aCallIndex });
  },

  conferenceCall: function conferenceCall(aClientId) {
    this._getClient(aClientId).sendWorkerMessage("conferenceCall");
  },

  separateCall: function separateCall(aClientId, aCallIndex) {
    this._getClient(aClientId).sendWorkerMessage("separateCall", { callIndex: aCallIndex });
  },

  holdConference: function holdConference(aClientId) {
    this._getClient(aClientId).sendWorkerMessage("holdConference");
  },

  resumeConference: function resumeConference(aClientId) {
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

  



  


  notifyCallDisconnected: function notifyCallDisconnected(aClientId, aCall) {
    if (DEBUG) debug("handleCallDisconnected: " + JSON.stringify(aCall));

    aCall.state = nsITelephonyProvider.CALL_STATE_DISCONNECTED;
    let duration = ("started" in aCall && typeof aCall.started == "number") ?
      new Date().getTime() - aCall.started : 0;
    let data = {
      number: aCall.number,
      duration: duration,
      direction: aCall.isOutgoing ? "outgoing" : "incoming"
    };
    gSystemMessenger.broadcastMessage("telephony-call-ended", data);

    this._updateCallAudioState(aCall, null);

    this._notifyAllListeners("callStateChanged", [aClientId,
                                                  aCall.callIndex,
                                                  aCall.state,
                                                  aCall.number,
                                                  aCall.isActive,
                                                  aCall.isOutgoing,
                                                  aCall.isEmergency,
                                                  aCall.isConference]);
  },

  


  notifyCallError: function notifyCallError(aClientId, aCallIndex, aErrorMsg) {
    this._notifyAllListeners("notifyError", [aClientId, aCallIndex, aErrorMsg]);
  },

  





  notifyCallRing: function notifyCallRing() {
    
    
    this._acquireCallRingWakeLock();

    gSystemMessenger.broadcastMessage("telephony-new-call", {});
  },

  



  notifyCallStateChanged: function notifyCallStateChanged(aClientId, aCall) {
    if (DEBUG) debug("handleCallStateChange: " + JSON.stringify(aCall));

    aCall.state = this._convertRILCallState(aCall.state);
    if (aCall.state == nsITelephonyProvider.CALL_STATE_DIALING) {
      gSystemMessenger.broadcastMessage("telephony-new-call", {});
    }

    this._updateCallAudioState(aCall, null);

    this._notifyAllListeners("callStateChanged", [aClientId,
                                                  aCall.callIndex,
                                                  aCall.state,
                                                  aCall.number,
                                                  aCall.isActive,
                                                  aCall.isOutgoing,
                                                  aCall.isEmergency,
                                                  aCall.isConference]);
  },

  notifyCdmaCallWaiting: function notifyCdmaCallWaiting(aClientId, aNumber) {
    
    
    this._acquireCallRingWakeLock();

    this._notifyAllListeners("notifyCdmaCallWaiting", [aClientId, aNumber]);
  },

  notifySupplementaryService:
      function notifySupplementaryService(aClientId, aCallIndex, aNotification) {
    let notification = this._convertRILSuppSvcNotification(aNotification);
    this._notifyAllListeners("supplementaryServiceNotification",
                             [aClientId, aCallIndex, notification]);
  },

  notifyConferenceCallStateChanged: function notifyConferenceCallStateChanged(aState) {
    if (DEBUG) debug("handleConferenceCallStateChanged: " + aState);
    aState = this._convertRILCallState(aState);
    this._updateCallAudioState(null, aState);

    this._notifyAllListeners("conferenceCallStateChanged", [aState]);
  },

  



  observe: function observe(aSubject, aTopic, aData) {
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
