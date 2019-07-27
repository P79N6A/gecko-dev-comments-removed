



"use strict";

const {classes: Cc, interfaces: Ci, utils: Cu, results: Cr} = Components;

Cu.import("resource://gre/modules/XPCOMUtils.jsm");

XPCOMUtils.defineLazyGetter(this, "RIL", function () {
  let obj = {};
  Cu.import("resource://gre/modules/ril_consts.js", obj);
  return obj;
});

XPCOMUtils.defineLazyGetter(this, "gStkCmdFactory", function() {
  let stk = {};
  Cu.import("resource://gre/modules/StkProactiveCmdFactory.jsm", stk);
  return stk.StkProactiveCmdFactory;
});




this.RILSystemMessenger = function() {};
RILSystemMessenger.prototype = {

  







  broadcastMessage: function(aType, aMessage) {
    
  },

  


  notifyNewCall: function() {
    this.broadcastMessage("telephony-new-call", {});
  },

  


  notifyCallEnded: function(aServiceId, aNumber, aCdmaWaitingNumber, aEmergency,
                            aDuration, aOutgoing, aHangUpLocal) {
    let data = {
      serviceId: aServiceId,
      number: aNumber,
      emergency: aEmergency,
      duration: aDuration,
      direction: aOutgoing ? "outgoing" : "incoming",
      hangUpLocal: aHangUpLocal
    };

    if (aCdmaWaitingNumber != null) {
      data.secondNumber = aCdmaWaitingNumber;
    }

    this.broadcastMessage("telephony-call-ended", data);
  },

  _convertSmsMessageClass: function(aMessageClass) {
    return RIL.GECKO_SMS_MESSAGE_CLASSES[aMessageClass] || null;
  },

  _convertSmsDelivery: function(aDelivery) {
    return ["received", "sending", "sent", "error"][aDelivery] || null;
  },

  _convertSmsDeliveryStatus: function(aDeliveryStatus) {
    return [
      RIL.GECKO_SMS_DELIVERY_STATUS_NOT_APPLICABLE,
      RIL.GECKO_SMS_DELIVERY_STATUS_SUCCESS,
      RIL.GECKO_SMS_DELIVERY_STATUS_PENDING,
      RIL.GECKO_SMS_DELIVERY_STATUS_ERROR
    ][aDeliveryStatus] || null;
  },

  



  notifySms: function(aNotificationType, aId, aThreadId, aIccId, aDelivery,
                      aDeliveryStatus, aSender, aReceiver, aBody, aMessageClass,
                      aTimestamp, aSentTimestamp, aDeliveryTimestamp, aRead) {
    let msgType = [
        "sms-received",
        "sms-sent",
        "sms-delivery-success",
        "sms-failed",
        "sms-delivery-error"
      ][aNotificationType];

    if (!msgType) {
      throw new Error("Invalid Notification Type: " + aNotificationType);
    }

    this.broadcastMessage(msgType, {
      iccId:             aIccId,
      type:              "sms",
      id:                aId,
      threadId:          aThreadId,
      delivery:          this._convertSmsDelivery(aDelivery),
      deliveryStatus:    this._convertSmsDeliveryStatus(aDeliveryStatus),
      sender:            aSender,
      receiver:          aReceiver,
      body:              aBody,
      messageClass:      this._convertSmsMessageClass(aMessageClass),
      timestamp:         aTimestamp,
      sentTimestamp:     aSentTimestamp,
      deliveryTimestamp: aDeliveryTimestamp,
      read:              aRead
    });
  },

  _convertCbGsmGeographicalScope: function(aGeographicalScope) {
    return RIL.CB_GSM_GEOGRAPHICAL_SCOPE_NAMES[aGeographicalScope] || null;
  },

  _convertCbMessageClass: function(aMessageClass) {
    return RIL.GECKO_SMS_MESSAGE_CLASSES[aMessageClass] || null;
  },

  _convertCbEtwsWarningType: function(aWarningType) {
    return RIL.CB_ETWS_WARNING_TYPE_NAMES[aWarningType] || null;
  },

  


  notifyCbMessageReceived: function(aServiceId, aGsmGeographicalScope, aMessageCode,
                                    aMessageId, aLanguage, aBody, aMessageClass,
                                    aTimestamp, aCdmaServiceCategory, aHasEtwsInfo,
                                    aEtwsWarningType, aEtwsEmergencyUserAlert, aEtwsPopup) {
    
    let data = {
      serviceId: aServiceId,
      gsmGeographicalScope: this._convertCbGsmGeographicalScope(aGsmGeographicalScope),
      messageCode: aMessageCode,
      messageId: aMessageId,
      language: aLanguage,
      body: aBody,
      messageClass: this._convertCbMessageClass(aMessageClass),
      timestamp: aTimestamp,
      cdmaServiceCategory: null,
      etws: null
    };

    if (aHasEtwsInfo) {
      data.etws = {
        warningType: this._convertCbEtwsWarningType(aEtwsWarningType),
        emergencyUserAlert: aEtwsEmergencyUserAlert,
        popup: aEtwsPopup
      };
    }

    if (aCdmaServiceCategory !=
        Ci.nsICellBroadcastService.CDMA_SERVICE_CATEGORY_INVALID) {
      data.cdmaServiceCategory = aCdmaServiceCategory;
    }

    this.broadcastMessage("cellbroadcast-received", data);
  },

  


  notifyUssdReceived: function(aServiceId, aMessage, aSessionEnded) {
    this.broadcastMessage("ussd-received", {
      serviceId: aServiceId,
      message: aMessage,
      sessionEnded: aSessionEnded
    });
  },

  


  notifyCdmaInfoRecDisplay: function(aServiceId, aDisplay) {
    this.broadcastMessage("cdma-info-rec-received", {
      clientId: aServiceId,
      display: aDisplay
    });
  },

  



  notifyCdmaInfoRecCalledPartyNumber: function(aServiceId, aType, aPlan,
                                               aNumber, aPi, aSi) {
    this.broadcastMessage("cdma-info-rec-received", {
      clientId: aServiceId,
      calledNumber: {
        type: aType,
        plan: aPlan,
        number: aNumber,
        pi: aPi,
        si: aSi
      }
    });
  },

  



  notifyCdmaInfoRecCallingPartyNumber: function(aServiceId, aType, aPlan,
                                                aNumber, aPi, aSi) {
    this.broadcastMessage("cdma-info-rec-received", {
      clientId: aServiceId,
      callingNumber: {
        type: aType,
        plan: aPlan,
        number: aNumber,
        pi: aPi,
        si: aSi
      }
    });
  },

  



  notifyCdmaInfoRecConnectedPartyNumber: function(aServiceId, aType, aPlan,
                                                  aNumber, aPi, aSi) {
    this.broadcastMessage("cdma-info-rec-received", {
      clientId: aServiceId,
      connectedNumber: {
        type: aType,
        plan: aPlan,
        number: aNumber,
        pi: aPi,
        si: aSi
      }
    });
  },

  


  notifyCdmaInfoRecSignal: function(aServiceId, aType, aAlertPitch, aSignal) {
    this.broadcastMessage("cdma-info-rec-received", {
      clientId: aServiceId,
      signal: {
        type: aType,
        alertPitch: aAlertPitch,
        signal: aSignal
      }
    });
  },

  



  notifyCdmaInfoRecRedirectingNumber: function(aServiceId, aType, aPlan,
                                               aNumber, aPi, aSi, aReason) {
    this.broadcastMessage("cdma-info-rec-received", {
      clientId: aServiceId,
      redirect: {
        type: aType,
        plan: aPlan,
        number: aNumber,
        pi: aPi,
        si: aSi,
        reason: aReason
      }
    });
  },

  


  notifyCdmaInfoRecLineControl: function(aServiceId, aPolarityIncluded,
                                         aToggle, aReverse, aPowerDenial) {
    this.broadcastMessage("cdma-info-rec-received", {
      clientId: aServiceId,
      lineControl: {
        polarityIncluded: aPolarityIncluded,
        toggle: aToggle,
        reverse: aReverse,
        powerDenial: aPowerDenial
      }
    });
  },

  


  notifyCdmaInfoRecClir: function(aServiceId, aCause) {
    this.broadcastMessage("cdma-info-rec-received", {
      clientId: aServiceId,
      clirCause: aCause
    });
  },

  


  notifyCdmaInfoRecAudioControl: function(aServiceId, aUpLink, aDownLink) {
    this.broadcastMessage("cdma-info-rec-received", {
      clientId: aServiceId,
      audioControl: {
        upLink: aUpLink,
        downLink: aDownLink
      }
    });
  },

  


  notifyStkProactiveCommand: function(aIccId, aCommand) {
    this.broadcastMessage("icc-stkcommand", {
      iccId: aIccId,
      command: gStkCmdFactory.createCommandMessage(aCommand)
    });
  }
};

this.EXPORTED_SYMBOLS = [
  'RILSystemMessenger'
];
