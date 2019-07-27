



"use strict";

const {classes: Cc, interfaces: Ci, utils: Cu, results: Cr} = Components;

Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://gre/modules/Services.jsm");

let RIL = {};
Cu.import("resource://gre/modules/ril_consts.js", RIL);

const GONK_SMSSERVICE_CONTRACTID = "@mozilla.org/sms/gonksmsservice;1";
const GONK_SMSSERVICE_CID = Components.ID("{f9b9b5e2-73b4-11e4-83ff-a33e27428c86}");

const NS_XPCOM_SHUTDOWN_OBSERVER_ID      = "xpcom-shutdown";
const NS_PREFBRANCH_PREFCHANGE_TOPIC_ID  = "nsPref:changed";

const kPrefDefaultServiceId = "dom.sms.defaultServiceId";
const kPrefRilDebuggingEnabled = "ril.debugging.enabled";
const kPrefRilNumRadioInterfaces = "ril.numRadioInterfaces";

const kSmsReceivedObserverTopic          = "sms-received";
const kSilentSmsReceivedObserverTopic    = "silent-sms-received";
const kSmsSendingObserverTopic           = "sms-sending";
const kSmsSentObserverTopic              = "sms-sent";
const kSmsFailedObserverTopic            = "sms-failed";
const kSmsDeliverySuccessObserverTopic   = "sms-delivery-success";
const kSmsDeliveryErrorObserverTopic     = "sms-delivery-error";

const DOM_MOBILE_MESSAGE_DELIVERY_RECEIVED = "received";
const DOM_MOBILE_MESSAGE_DELIVERY_SENDING  = "sending";
const DOM_MOBILE_MESSAGE_DELIVERY_SENT     = "sent";
const DOM_MOBILE_MESSAGE_DELIVERY_ERROR    = "error";

XPCOMUtils.defineLazyGetter(this, "gRadioInterfaces", function() {
  let ril = Cc["@mozilla.org/ril;1"].getService(Ci.nsIRadioInterfaceLayer);

  let interfaces = [];
  for (let i = 0; i < ril.numRadioInterfaces; i++) {
    interfaces.push(ril.getRadioInterface(i));
  }
  return interfaces;
});

XPCOMUtils.defineLazyGetter(this, "gSmsSegmentHelper", function() {
  let ns = {};
  Cu.import("resource://gre/modules/SmsSegmentHelper.jsm", ns);
  return ns.SmsSegmentHelper;
});

XPCOMUtils.defineLazyGetter(this, "gPhoneNumberUtils", function() {
  let ns = {};
  Cu.import("resource://gre/modules/PhoneNumberUtils.jsm", ns);
  return ns.PhoneNumberUtils;
});

XPCOMUtils.defineLazyServiceGetter(this, "gMobileConnectionService",
                                   "@mozilla.org/mobileconnection/mobileconnectionservice;1",
                                   "nsIMobileConnectionService");

XPCOMUtils.defineLazyServiceGetter(this, "gMobileMessageDatabaseService",
                                   "@mozilla.org/mobilemessage/rilmobilemessagedatabaseservice;1",
                                   "nsIRilMobileMessageDatabaseService");

XPCOMUtils.defineLazyServiceGetter(this, "gMobileMessageService",
                                   "@mozilla.org/mobilemessage/mobilemessageservice;1",
                                   "nsIMobileMessageService");

XPCOMUtils.defineLazyServiceGetter(this, "gSmsMessenger",
                                   "@mozilla.org/ril/system-messenger-helper;1",
                                   "nsISmsMessenger");

let DEBUG = RIL.DEBUG_RIL;
function debug(s) {
  dump("SmsService: " + s);
}

function SmsService() {
  this._silentNumbers = [];
  this.smsDefaultServiceId = this._getDefaultServiceId();

  Services.prefs.addObserver(kPrefRilDebuggingEnabled, this, false);
  Services.prefs.addObserver(kPrefDefaultServiceId, this, false);
  Services.obs.addObserver(this, NS_XPCOM_SHUTDOWN_OBSERVER_ID, false);
}
SmsService.prototype = {
  classID: GONK_SMSSERVICE_CID,

  classInfo: XPCOMUtils.generateCI({classID: GONK_SMSSERVICE_CID,
                                    contractID: GONK_SMSSERVICE_CONTRACTID,
                                    classDescription: "SmsService",
                                    interfaces: [Ci.nsISmsService,
                                                 Ci.nsIGonkSmsService],
                                    flags: Ci.nsIClassInfo.SINGLETON}),

  QueryInterface: XPCOMUtils.generateQI([Ci.nsISmsService,
                                         Ci.nsIGonkSmsService,
                                         Ci.nsIObserver]),

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

  _getPhoneNumber: function(aServiceId) {
    let number;
    
    try {
      let iccInfo = null;
      let baseIccInfo = gRadioInterfaces[aServiceId].rilContext.iccInfo;
      if (baseIccInfo.iccType === 'ruim' || baseIccInfo.iccType === 'csim') {
        iccInfo = baseIccInfo.QueryInterface(Ci.nsICdmaIccInfo);
        number = iccInfo.mdn;
      } else {
        iccInfo = baseIccInfo.QueryInterface(Ci.nsIGsmIccInfo);
        number = iccInfo.msisdn;
      }
    } catch (e) {
      if (DEBUG) {
       debug("Exception - QueryInterface failed on iccinfo for GSM/CDMA info");
      }
      return null;
    }

    return number;
  },

  _getIccId: function(aServiceId) {
    let iccInfo = gRadioInterfaces[aServiceId].rilContext.iccInfo;

    if (!iccInfo) {
      return null;
    }

    return iccInfo.iccid;
  },

  _convertSmsMessageClass: function(aMessageClass) {
    let index = RIL.GECKO_SMS_MESSAGE_CLASSES.indexOf(aMessageClass);

    if (index < 0) {
      throw new Error("Invalid MessageClass: " + aMessageClass);
    }

    return index;
  },

  _convertSmsDelivery: function(aDelivery) {
    let index = [DOM_MOBILE_MESSAGE_DELIVERY_RECEIVED,
                 DOM_MOBILE_MESSAGE_DELIVERY_SENDING,
                 DOM_MOBILE_MESSAGE_DELIVERY_SENT,
                 DOM_MOBILE_MESSAGE_DELIVERY_ERROR].indexOf(aDelivery);

    if (index < 0) {
      throw new Error("Invalid Delivery: " + aDelivery);
    }

    return index;
  },

  _convertSmsDeliveryStatus: function(aDeliveryStatus) {
    let index = [RIL.GECKO_SMS_DELIVERY_STATUS_NOT_APPLICABLE,
                 RIL.GECKO_SMS_DELIVERY_STATUS_SUCCESS,
                 RIL.GECKO_SMS_DELIVERY_STATUS_PENDING,
                 RIL.GECKO_SMS_DELIVERY_STATUS_ERROR].indexOf(aDeliveryStatus);

    if (index < 0) {
      throw new Error("Invalid DeliveryStatus: " + aDeliveryStatus);
    }

    return index;
  },

  _sendToTheAir: function(aServiceId, aDomMessage, aSilent, aOptions, aRequest) {
    
    let sentMessage = aDomMessage;
    let requestStatusReport = aOptions.requestStatusReport;

    gRadioInterfaces[aServiceId].sendWorkerMessage("sendSMS",
                                                   aOptions,
                                                   (aResponse) => {
      
      if (aResponse.errorMsg) {
        let error = Ci.nsIMobileMessageCallback.UNKNOWN_ERROR;
        switch (aResponse.errorMsg) {
          case RIL.ERROR_RADIO_NOT_AVAILABLE:
            error = Ci.nsIMobileMessageCallback.NO_SIGNAL_ERROR;
            break;
          case RIL.ERROR_FDN_CHECK_FAILURE:
            error = Ci.nsIMobileMessageCallback.FDN_CHECK_ERROR;
            break;
        }

        if (aSilent) {
          
          
          
          aRequest.notifySendMessageFailed(
            error,
            gMobileMessageService.createSmsMessage(sentMessage.id,
                                                   sentMessage.threadId,
                                                   sentMessage.iccId,
                                                   DOM_MOBILE_MESSAGE_DELIVERY_ERROR,
                                                   RIL.GECKO_SMS_DELIVERY_STATUS_ERROR,
                                                   sentMessage.sender,
                                                   sentMessage.receiver,
                                                   sentMessage.body,
                                                   sentMessage.messageClass,
                                                   sentMessage.timestamp,
                                                   0,
                                                   0,
                                                   sentMessage.read));
          return false;
        }

        gMobileMessageDatabaseService
          .setMessageDeliveryByMessageId(aDomMessage.id,
                                         null,
                                         DOM_MOBILE_MESSAGE_DELIVERY_ERROR,
                                         RIL.GECKO_SMS_DELIVERY_STATUS_ERROR,
                                         null,
                                         (aRv, aDomMessage) => {
          
          aRequest.notifySendMessageFailed(error, aDomMessage);
          Services.obs.notifyObservers(aDomMessage, kSmsFailedObserverTopic, null);
        });
        return false;
      } 

      
      if (!aResponse.deliveryStatus) {
        if (aSilent) {
          
          
          
          aRequest.notifyMessageSent(
            gMobileMessageService.createSmsMessage(sentMessage.id,
                                                   sentMessage.threadId,
                                                   sentMessage.iccId,
                                                   DOM_MOBILE_MESSAGE_DELIVERY_SENT,
                                                   sentMessage.deliveryStatus,
                                                   sentMessage.sender,
                                                   sentMessage.receiver,
                                                   sentMessage.body,
                                                   sentMessage.messageClass,
                                                   sentMessage.timestamp,
                                                   Date.now(),
                                                   0,
                                                   sentMessage.read));
          
          return false;
        }

        gMobileMessageDatabaseService
          .setMessageDeliveryByMessageId(sentMessage.id,
                                         null,
                                         DOM_MOBILE_MESSAGE_DELIVERY_SENT,
                                         sentMessage.deliveryStatus,
                                         null,
                                         (aRv, aDomMessage) => {
          

          if (requestStatusReport) {
            
            sentMessage = aDomMessage;
          }

          this._broadcastSmsSystemMessage(
            Ci.nsISmsMessenger.NOTIFICATION_TYPE_SENT, aDomMessage);
          aRequest.notifyMessageSent(aDomMessage);
          Services.obs.notifyObservers(aDomMessage, kSmsSentObserverTopic, null);
        });

        
        return requestStatusReport;
      } 

      
      
      gMobileMessageDatabaseService
        .setMessageDeliveryByMessageId(sentMessage.id,
                                       null,
                                       sentMessage.delivery,
                                       aResponse.deliveryStatus,
                                       null,
                                       (aRv, aDomMessage) => {
        

        let topic = (aResponse.deliveryStatus ==
                     RIL.GECKO_SMS_DELIVERY_STATUS_SUCCESS)
                    ? kSmsDeliverySuccessObserverTopic
                    : kSmsDeliveryErrorObserverTopic;

        
        if (topic == kSmsDeliverySuccessObserverTopic) {
          this._broadcastSmsSystemMessage(
            Ci.nsISmsMessenger.NOTIFICATION_TYPE_DELIVERY_SUCCESS, aDomMessage);
        }

        
        Services.obs.notifyObservers(aDomMessage, topic, null);
      });

      
      return false;
    });
  },

  








  _broadcastSmsSystemMessage: function(aNotificationType, aDomMessage) {
    if (DEBUG) debug("Broadcasting the SMS system message: " + aNotificationType);

    
    
    
    try {
      gSmsMessenger.notifySms(aNotificationType,
                              aDomMessage.id,
                              aDomMessage.threadId,
                              aDomMessage.iccId,
                              this._convertSmsDelivery(
                                aDomMessage.delivery),
                              this._convertSmsDeliveryStatus(
                                aDomMessage.deliveryStatus),
                              aDomMessage.sender,
                              aDomMessage.receiver,
                              aDomMessage.body,
                              this._convertSmsMessageClass(
                                aDomMessage.messageClass),
                              aDomMessage.timestamp,
                              aDomMessage.sentTimestamp,
                              aDomMessage.deliveryTimestamp,
                              aDomMessage.read);
    } catch (e) {
      if (DEBUG) {
        debug("Failed to _broadcastSmsSystemMessage: " + e);
      }
    }
  },

  


  smsDefaultServiceId: 0,

  getSegmentInfoForText: function(aText, aRequest) {
    let strict7BitEncoding;
    try {
      strict7BitEncoding = Services.prefs.getBoolPref("dom.sms.strict7BitEncoding");
    } catch (e) {
      strict7BitEncoding = false;
    }

    let options = gSmsSegmentHelper.fragmentText(aText, null, strict7BitEncoding);
    let charsInLastSegment;
    if (options.segmentMaxSeq) {
      let lastSegment = options.segments[options.segmentMaxSeq - 1];
      charsInLastSegment = lastSegment.encodedBodyLength;
      if (options.dcs == RIL.PDU_DCS_MSG_CODING_16BITS_ALPHABET) {
        
        charsInLastSegment /= 2;
      }
    } else {
      charsInLastSegment = 0;
    }

    aRequest.notifySegmentInfoForTextGot(options.segmentMaxSeq,
                                         options.segmentChars,
                                         options.segmentChars - charsInLastSegment);
  },

  send: function(aServiceId, aNumber, aMessage, aSilent, aRequest) {
    if (aServiceId > (gRadioInterfaces.length - 1)) {
      throw Cr.NS_ERROR_INVALID_ARG;
    }

    let strict7BitEncoding;
    try {
      strict7BitEncoding = Services.prefs.getBoolPref("dom.sms.strict7BitEncoding");
    } catch (e) {
      strict7BitEncoding = false;
    }

    let options = gSmsSegmentHelper.fragmentText(aMessage, null, strict7BitEncoding);
    options.number = gPhoneNumberUtils.normalize(aNumber);
    let requestStatusReport;
    try {
      requestStatusReport =
        Services.prefs.getBoolPref("dom.sms.requestStatusReport");
    } catch (e) {
      requestStatusReport = true;
    }
    options.requestStatusReport = requestStatusReport && !aSilent;

    let sendingMessage = {
      type: "sms",
      sender: this._getPhoneNumber(aServiceId),
      receiver: aNumber,
      body: aMessage,
      deliveryStatusRequested: options.requestStatusReport,
      timestamp: Date.now(),
      iccId: this._getIccId(aServiceId)
    };

    let saveSendingMessageCallback = (aRv, aSendingMessage) => {
      if (!Components.isSuccessCode(aRv)) {
        if (DEBUG) debug("Error! Fail to save sending message! aRv = " + aRv);
        aRequest.notifySendMessageFailed(
          gMobileMessageDatabaseService.translateCrErrorToMessageCallbackError(aRv),
          aSendingMessage);
        Services.obs.notifyObservers(aSendingMessage, kSmsFailedObserverTopic, null);
        return;
      }

      if (!aSilent) {
        Services.obs.notifyObservers(aSendingMessage, kSmsSendingObserverTopic, null);
      }

      let connection =
        gMobileConnectionService.getItemByServiceId(aServiceId);
      
      
      let errorCode;
      let radioState = connection && connection.radioState;
      if (!gPhoneNumberUtils.isPlainPhoneNumber(options.number)) {
        if (DEBUG) debug("Error! Address is invalid when sending SMS: " + options.number);
        errorCode = Ci.nsIMobileMessageCallback.INVALID_ADDRESS_ERROR;
      } else if (radioState == Ci.nsIMobileConnection.MOBILE_RADIO_STATE_UNKNOWN ||
                 radioState == Ci.nsIMobileConnection.MOBILE_RADIO_STATE_DISABLED) {
        if (DEBUG) debug("Error! Radio is disabled when sending SMS.");
        errorCode = Ci.nsIMobileMessageCallback.RADIO_DISABLED_ERROR;
      } else if (gRadioInterfaces[aServiceId].rilContext.cardState !=
                 Ci.nsIIccProvider.CARD_STATE_READY) {
        if (DEBUG) debug("Error! SIM card is not ready when sending SMS.");
        errorCode = Ci.nsIMobileMessageCallback.NO_SIM_CARD_ERROR;
      }
      if (errorCode) {
        if (aSilent) {
          aRequest.notifySendMessageFailed(errorCode, aSendingMessage);
          return;
        }

        gMobileMessageDatabaseService
          .setMessageDeliveryByMessageId(aSendingMessage.id,
                                         null,
                                         DOM_MOBILE_MESSAGE_DELIVERY_ERROR,
                                         RIL.GECKO_SMS_DELIVERY_STATUS_ERROR,
                                         null,
                                         (aRv, aDomMessage) => {
          
          aRequest.notifySendMessageFailed(errorCode, aDomMessage);
          Services.obs.notifyObservers(aDomMessage, kSmsFailedObserverTopic, null);
        });
        return;
      }

      this._sendToTheAir(aServiceId, aSendingMessage, aSilent, options, aRequest);
    }; 

    
    if (aSilent) {
      let delivery = DOM_MOBILE_MESSAGE_DELIVERY_SENDING;
      let deliveryStatus = RIL.GECKO_SMS_DELIVERY_STATUS_PENDING;
      let domMessage =
        gMobileMessageService.createSmsMessage(-1, 
                                               0,  
                                               sendingMessage.iccId,
                                               delivery,
                                               deliveryStatus,
                                               sendingMessage.sender,
                                               sendingMessage.receiver,
                                               sendingMessage.body,
                                               "normal", 
                                               sendingMessage.timestamp,
                                               0,
                                               0,
                                               false);
      saveSendingMessageCallback(Cr.NS_OK, domMessage);
      return;
    }

    gMobileMessageDatabaseService.saveSendingMessage(
      sendingMessage, saveSendingMessageCallback);
  },

  
  _silentNumbers: null,
  isSilentNumber: function(aNumber) {
    return this._silentNumbers.indexOf(aNumber) >= 0;
  },

  addSilentNumber: function(aNumber) {
    if (this.isSilentNumber(aNumber)) {
      throw Cr.NS_ERROR_UNEXPECTED;
    }

    this._silentNumbers.push(aNumber);
  },

  removeSilentNumber: function(aNumber) {
   let index = this._silentNumbers.indexOf(aNumber);
   if (index < 0) {
     throw Cr.NS_ERROR_INVALID_ARG;
   }

   this._silentNumbers.splice(index, 1);
  },

  getSmscAddress: function(aServiceId, aRequest) {
    if (aServiceId > (gRadioInterfaces.length - 1)) {
      throw Cr.NS_ERROR_INVALID_ARG;
    }

    gRadioInterfaces[aServiceId].sendWorkerMessage("getSmscAddress",
                                                   null,
                                                   (aResponse) => {
      if (!aResponse.errorMsg) {
        aRequest.notifyGetSmscAddress(aResponse.smscAddress);
      } else {
        aRequest.notifyGetSmscAddressFailed(
          Ci.nsIMobileMessageCallback.NOT_FOUND_ERROR);
      }
    });
  },

  



  


  observe: function(aSubject, aTopic, aData) {
    switch (aTopic) {
      case NS_PREFBRANCH_PREFCHANGE_TOPIC_ID:
        if (aData === kPrefRilDebuggingEnabled) {
          this._updateDebugFlag();
        }
        else if (aData === kPrefDefaultServiceId) {
          this.smsDefaultServiceId = this._getDefaultServiceId();
        }
        break;
      case NS_XPCOM_SHUTDOWN_OBSERVER_ID:
        Services.prefs.removeObserver(kPrefRilDebuggingEnabled, this);
        Services.prefs.removeObserver(kPrefDefaultServiceId, this);
        Services.obs.removeObserver(this, NS_XPCOM_SHUTDOWN_OBSERVER_ID);
        break;
    }
  }
};

this.NSGetFactory = XPCOMUtils.generateNSGetFactory([SmsService]);
