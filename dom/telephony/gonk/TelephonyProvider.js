




"use strict";

const {classes: Cc, interfaces: Ci, utils: Cu, results: Cr} = Components;

Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://gre/modules/Services.jsm");

var RIL = {};
Cu.import("resource://gre/modules/ril_consts.js", RIL);

const GONK_TELEPHONYPROVIDER_CONTRACTID =
  "@mozilla.org/telephony/gonktelephonyprovider;1";
const GONK_TELEPHONYPROVIDER_CID =
  Components.ID("{67d26434-d063-4d28-9f48-5b3189788155}");

const kPrefenceChangedObserverTopic = "nsPref:changed";
const kXpcomShutdownObserverTopic   = "xpcom-shutdown";

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

XPCOMUtils.defineLazyServiceGetter(this, "gPowerManagerService",
                                   "@mozilla.org/power/powermanagerservice;1",
                                   "nsIPowerManagerService");

XPCOMUtils.defineLazyServiceGetter(this, "gSystemMessenger",
                                   "@mozilla.org/system-message-internal;1",
                                   "nsISystemMessagesInternal");

XPCOMUtils.defineLazyGetter(this, "gRadioInterface", function () {
  let ril = Cc["@mozilla.org/ril;1"].getService(Ci["nsIRadioInterfaceLayer"]);
  
  return ril.getRadioInterface(0);
});

XPCOMUtils.defineLazyGetter(this, "gPhoneNumberUtils", function () {
  let ns = {};
  Cu.import("resource://gre/modules/PhoneNumberUtils.jsm", ns);
  return ns.PhoneNumberUtils;
});

function TelephonyProvider() {
  this._listeners = [];

  this._updateDebugFlag();

  Services.obs.addObserver(this, kPrefenceChangedObserverTopic, false);
  Services.obs.addObserver(this, kXpcomShutdownObserverTopic, false);
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
  _cancelCallRingWakeLockTimer: function _cancelCallRingWakeLockTimer() {
    if (this._callRingWakeLockTimer) {
      this._callRingWakeLockTimer.cancel();
    }
    if (this._callRingWakeLock) {
      this._callRingWakeLock.unlock();
      this._callRingWakeLock = null;
    }
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
      if (this._activeCall && this._activeCall.callIndex == aCall.callIndex) {
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
        if (this._activeCall &&
            this._activeCall.callIndex == aCall.callIndex) {
          
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
              Services.prefs.getBoolPref("ril.debugging.enabled");
    } catch (e) {}
  },

  



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
    gRadioInterface.sendWorkerMessage("enumerateCalls", null,
                                      (function(response) {
      for (let call of response.calls) {
        call.state = this._convertRILCallState(call.state);
        call.isActive = this._activeCall ?
          (call.callIndex == this._activeCall.callIndex) : false;

        aListener.enumerateCallState(call.callIndex, call.state, call.number,
                                     call.isActive, call.isOutgoing,
                                     call.isEmergency, call.isConference);
      }
      aListener.enumerateCallStateComplete();

      return false;
    }).bind(this));
  },

  dial: function(aNumber, aIsEmergency) {
    if (DEBUG) debug("Dialing " + (aIsEmergency ? "emergency " : "") + aNumber);
    
    
    if (!aIsEmergency) {
      aNumber = gPhoneNumberUtils.normalize(aNumber);
    }
    if (this._validateNumber(aNumber)) {
      gRadioInterface.sendWorkerMessage("dial", { number: aNumber,
                                                  isDialEmergency: aIsEmergency });
    }
  },

  hangUp: function(aCallIndex) {
    gRadioInterface.sendWorkerMessage("hangUp", { callIndex: aCallIndex });
  },

  startTone: function(aDtmfChar) {
    gRadioInterface.sendWorkerMessage("startTone", { dtmfChar: aDtmfChar });
  },

  stopTone: function() {
    gRadioInterface.sendWorkerMessage("stopTone");
  },

  answerCall: function(aCallIndex) {
    gRadioInterface.sendWorkerMessage("answerCall", { callIndex: aCallIndex });
  },

  rejectCall: function(aCallIndex) {
    gRadioInterface.sendWorkerMessage("rejectCall", { callIndex: aCallIndex });
  },

  holdCall: function(aCallIndex) {
    gRadioInterface.sendWorkerMessage("holdCall", { callIndex: aCallIndex });
  },

  resumeCall: function(aCallIndex) {
    gRadioInterface.sendWorkerMessage("resumeCall", { callIndex: aCallIndex });
  },

  conferenceCall: function conferenceCall() {
    gRadioInterface.sendWorkerMessage("conferenceCall");
  },

  separateCall: function separateCall(aCallIndex) {
    gRadioInterface.sendWorkerMessage("separateCall", { callIndex: aCallIndex });
  },

  holdConference: function holdConference() {
    gRadioInterface.sendWorkerMessage("holdConference");
  },

  resumeConference: function resumeConference() {
    gRadioInterface.sendWorkerMessage("resumeConference");
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

  



  


  notifyCallDisconnected: function notifyCallDisconnected(aCall) {
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

    this._notifyAllListeners("callStateChanged", [aCall.callIndex,
                                                  aCall.state,
                                                  aCall.number,
                                                  aCall.isActive,
                                                  aCall.isOutgoing,
                                                  aCall.isEmergency,
                                                  aCall.isConference]);
  },

  


  notifyCallError: function notifyCallError(aCallIndex, aErrorMsg) {
    this._notifyAllListeners("notifyError", [aCallIndex, aErrorMsg]);
  },

  





  notifyCallRing: function notifyCallRing() {
    if (!this._callRingWakeLock) {
      this._callRingWakeLock = gPowerManagerService.newWakeLock("cpu");
    }
    if (!this._callRingWakeLockTimer) {
      this._callRingWakeLockTimer =
        Cc["@mozilla.org/timer;1"].createInstance(Ci.nsITimer);
    }
    this._callRingWakeLockTimer
        .initWithCallback(this._cancelCallRingWakeLockTimer.bind(this),
                          CALL_WAKELOCK_TIMEOUT, Ci.nsITimer.TYPE_ONE_SHOT);

    gSystemMessenger.broadcastMessage("telephony-new-call", {});
  },

  



  notifyCallStateChanged: function notifyCallStateChanged(aCall) {
    if (DEBUG) debug("handleCallStateChange: " + JSON.stringify(aCall));

    aCall.state = this._convertRILCallState(aCall.state);
    if (aCall.state == nsITelephonyProvider.CALL_STATE_DIALING) {
      gSystemMessenger.broadcastMessage("telephony-new-call", {});
    }

    this._updateCallAudioState(aCall, null);

    this._notifyAllListeners("callStateChanged", [aCall.callIndex,
                                                  aCall.state,
                                                  aCall.number,
                                                  aCall.isActive,
                                                  aCall.isOutgoing,
                                                  aCall.isEmergency,
                                                  aCall.isConference]);
  },

  notifyCdmaCallWaiting: function notifyCdmaCallWaiting(aNumber) {
    this._notifyAllListeners("notifyCdmaCallWaiting", [aNumber]);
  },

  notifySupplementaryService: function notifySupplementaryService(aCallIndex,
                                                                  aNotification) {
    let notification = this._convertRILSuppSvcNotification(aNotification);
    this._notifyAllListeners("supplementaryServiceNotification",
                             [aCallIndex, notification]);
  },

  notifyConferenceCallStateChanged: function notifyConferenceCallStateChanged(aState) {
    if (DEBUG) debug("handleConferenceCallStateChanged: " + aState);
    aState = this._convertRILCallState(aState);
    this._updateCallAudioState(null, aState);

    this._notifyAllListeners("conferenceCallStateChanged", [aState]);
  },

  



  observe: function observe(aSubject, aTopic, aData) {
    switch (aTopic) {
      case kPrefenceChangedObserverTopic:
        if (aData === "ril.debugging.enabled") {
          this._updateDebugFlag();
        }
        break;

      case kXpcomShutdownObserverTopic:
        
        this._cancelCallRingWakeLockTimer();

        Services.obs.removeObserver(this, kPrefenceChangedObserverTopic);
        Services.obs.removeObserver(this, kXpcomShutdownObserverTopic);
        break;
    }
  }
};

this.NSGetFactory = XPCOMUtils.generateNSGetFactory([TelephonyProvider]);
