



"use strict";

this.EXPORTED_SYMBOLS = ["TelephonyUtils"];

const {classes: Cc, interfaces: Ci, utils: Cu, results: Cr} = Components;

Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://gre/modules/Promise.jsm");


XPCOMUtils.defineLazyServiceGetter(this,
                                   "TelephonyService",
                                   "@mozilla.org/telephony/telephonyservice;1",
                                   "nsITelephonyService");

function getCurrentCalls(aFilter) {
 if (aFilter === undefined) {
   aFilter = call => true;
 }

 let calls = [];

 
 TelephonyService.enumerateCalls({
   QueryInterface: XPCOMUtils.generateQI([Ci.nsITelephonyListener]),
   enumerateCallStateComplete: function() {},
   enumerateCallState: function(call) {
     if (aFilter(call)) {
       calls.push(call);
     }
   },
 });

 return calls;
}

this.TelephonyUtils = {
  





  hasAnyCalls: function(aClientId) {
    let calls = getCurrentCalls(call => {
      if (aClientId !== undefined && call.clientId !== aClientId) {
        return false;
      }
      return true;
    });

    return calls.length !== 0;
  },

  





  hasConnectedCalls: function(aClientId) {
    let calls = getCurrentCalls(call => {
      if (aClientId !== undefined && call.clientId !== aClientId) {
        return false;
      }
      return call.callState === Ci.nsITelephonyService.CALL_STATE_CONNECTED;
    });

    return calls.length !== 0;
  },

  





  waitForNoCalls: function(aClientId) {
    if (!this.hasAnyCalls(aClientId)) {
      return Promise.resolve();
    }

    let self = this;
    return new Promise(resolve => {
      let listener = {
        QueryInterface: XPCOMUtils.generateQI([Ci.nsITelephonyListener]),

        enumerateCallStateComplete: function() {},
        enumerateCallState: function() {},
        callStateChanged: function() {
          if (!self.hasAnyCalls(aClientId)) {
            TelephonyService.unregisterListener(this);
            resolve();
          }
        },
        conferenceCallStateChanged: function() {},
        supplementaryServiceNotification: function() {},
        notifyError: function() {},
        notifyCdmaCallWaiting: function() {},
        notifyConferenceError: function() {}
      };

      TelephonyService.registerListener(listener);
    });
  }
};
