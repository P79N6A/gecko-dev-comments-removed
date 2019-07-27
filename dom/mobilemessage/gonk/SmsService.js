



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
const kPrefLastKnownSimMcc = "ril.lastKnownSimMcc";

const kDiskSpaceWatcherObserverTopic = "disk-space-watcher";

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

const SMS_HANDLED_WAKELOCK_TIMEOUT = 5000;

XPCOMUtils.defineLazyGetter(this, "gRadioInterfaces", function() {
  let ril = { numRadioInterfaces: 0 };
  try {
    ril = Cc["@mozilla.org/ril;1"].getService(Ci.nsIRadioInterfaceLayer);
  } catch(e) {}

  let interfaces = [];
  for (let i = 0; i < ril.numRadioInterfaces; i++) {
    interfaces.push(ril.getRadioInterface(i));
  }
  return interfaces;
});

XPCOMUtils.defineLazyGetter(this, "gSmsSegmentHelper", function() {
  let ns = {};
  Cu.import("resource://gre/modules/SmsSegmentHelper.jsm", ns);

  
  ns.SmsSegmentHelper.enabledGsmTableTuples = getEnabledGsmTableTuplesFromMcc();

  return ns.SmsSegmentHelper;
});

XPCOMUtils.defineLazyGetter(this, "gPhoneNumberUtils", function() {
  let ns = {};
  Cu.import("resource://gre/modules/PhoneNumberUtils.jsm", ns);
  return ns.PhoneNumberUtils;
});

XPCOMUtils.defineLazyGetter(this, "gWAP", function() {
  let ns = {};
  Cu.import("resource://gre/modules/WapPushManager.js", ns);
  return ns;
});

XPCOMUtils.defineLazyServiceGetter(this, "gCellBroadcastService",
                                   "@mozilla.org/cellbroadcast/gonkservice;1",
                                   "nsIGonkCellBroadcastService");

XPCOMUtils.defineLazyServiceGetter(this, "gMobileConnectionService",
                                   "@mozilla.org/mobileconnection/mobileconnectionservice;1",
                                   "nsIMobileConnectionService");

XPCOMUtils.defineLazyServiceGetter(this, "gMobileMessageDatabaseService",
                                   "@mozilla.org/mobilemessage/gonkmobilemessagedatabaseservice;1",
                                   "nsIGonkMobileMessageDatabaseService");

XPCOMUtils.defineLazyServiceGetter(this, "gMobileMessageService",
                                   "@mozilla.org/mobilemessage/mobilemessageservice;1",
                                   "nsIMobileMessageService");

XPCOMUtils.defineLazyServiceGetter(this, "gPowerManagerService",
                                   "@mozilla.org/power/powermanagerservice;1",
                                   "nsIPowerManagerService");

XPCOMUtils.defineLazyServiceGetter(this, "gSmsMessenger",
                                   "@mozilla.org/ril/system-messenger-helper;1",
                                   "nsISmsMessenger");

let DEBUG = RIL.DEBUG_RIL;
function debug(s) {
  dump("SmsService: " + s);
}

function SmsService() {
  this._updateDebugFlag();
  this._silentNumbers = [];
  this.smsDefaultServiceId = this._getDefaultServiceId();

  this._portAddressedSmsApps = {};
  this._portAddressedSmsApps[gWAP.WDP_PORT_PUSH] =
    (aMessage, aServiceId) => this._handleSmsWdpPortPush(aMessage, aServiceId);

  this._receivedSmsSegmentsMap = {};

  Services.prefs.addObserver(kPrefRilDebuggingEnabled, this, false);
  Services.prefs.addObserver(kPrefDefaultServiceId, this, false);
  Services.prefs.addObserver(kPrefLastKnownSimMcc, this, false);
  Services.obs.addObserver(this, NS_XPCOM_SHUTDOWN_OBSERVER_ID, false);
  Services.obs.addObserver(this, kDiskSpaceWatcherObserverTopic, false);
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
      DEBUG = DEBUG ||
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

  
  
  
  _smsHandledWakeLock: null,
  _smsHandledWakeLockTimer: null,
  _acquireSmsHandledWakeLock: function() {
    if (!this._smsHandledWakeLock) {
      if (DEBUG) debug("Acquiring a CPU wake lock for handling SMS.");
      this._smsHandledWakeLock = gPowerManagerService.newWakeLock("cpu");
    }
    if (!this._smsHandledWakeLockTimer) {
      if (DEBUG) debug("Creating a timer for releasing the CPU wake lock.");
      this._smsHandledWakeLockTimer =
        Cc["@mozilla.org/timer;1"].createInstance(Ci.nsITimer);
    }
    if (DEBUG) debug("Setting the timer for releasing the CPU wake lock.");
    this._smsHandledWakeLockTimer
        .initWithCallback(() => this._releaseSmsHandledWakeLock(),
                          SMS_HANDLED_WAKELOCK_TIMEOUT,
                          Ci.nsITimer.TYPE_ONE_SHOT);
  },

  _releaseSmsHandledWakeLock: function() {
    if (DEBUG) debug("Releasing the CPU wake lock for handling SMS.");
    if (this._smsHandledWakeLockTimer) {
      this._smsHandledWakeLockTimer.cancel();
    }
    if (this._smsHandledWakeLock) {
      this._smsHandledWakeLock.unlock();
      this._smsHandledWakeLock = null;
    }
  },

  _convertSmsMessageClassToString: function(aMessageClass) {
    return RIL.GECKO_SMS_MESSAGE_CLASSES[aMessageClass] || null;
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
          
          this._broadcastSmsSystemMessage(
            Ci.nsISmsMessenger.NOTIFICATION_TYPE_SENT_FAILED, aDomMessage);
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
        

        let [topic, notificationType] =
          (aResponse.deliveryStatus == RIL.GECKO_SMS_DELIVERY_STATUS_SUCCESS)
            ? [kSmsDeliverySuccessObserverTopic,
               Ci.nsISmsMessenger.NOTIFICATION_TYPE_DELIVERY_SUCCESS]
            : [kSmsDeliveryErrorObserverTopic,
               Ci.nsISmsMessenger.NOTIFICATION_TYPE_DELIVERY_ERROR];

        
        
        this._broadcastSmsSystemMessage(notificationType, aDomMessage);

        
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

  










  _receivedSmsSegmentsMap: null,
  _processReceivedSmsSegment: function(aSegment) {
    
    if (!(aSegment.segmentMaxSeq && (aSegment.segmentMaxSeq > 1))) {
      if (aSegment.encoding == Ci.nsIGonkSmsService.SMS_MESSAGE_ENCODING_8BITS_ALPHABET) {
        aSegment.fullData = aSegment.data;
      } else {
        aSegment.fullBody = aSegment.body;
      }
      return aSegment;
    }

    
    let hash = aSegment.sender + ":" +
               aSegment.segmentRef + ":" +
               aSegment.segmentMaxSeq;
    let seq = aSegment.segmentSeq;

    let options = this._receivedSmsSegmentsMap[hash];
    if (!options) {
      options = aSegment;
      this._receivedSmsSegmentsMap[hash] = options;

      options.receivedSegments = 0;
      options.segments = [];
    } else if (options.segments[seq]) {
      
      if (DEBUG) {
        debug("Got duplicated segment no." + seq +
              " of a multipart SMS: " + JSON.stringify(aSegment));
      }
      return null;
    }

    if (options.receivedSegments > 0) {
      
      options.timestamp = aSegment.timestamp;
    }

    if (options.encoding == Ci.nsIGonkSmsService.SMS_MESSAGE_ENCODING_8BITS_ALPHABET) {
      options.segments[seq] = aSegment.data;
    } else {
      options.segments[seq] = aSegment.body;
    }
    options.receivedSegments++;

    
    
    
    
    
    if (aSegment.teleservice === RIL.PDU_CDMA_MSG_TELESERIVCIE_ID_WAP
        && seq === 1) {
      if (options.originatorPort === Ci.nsIGonkSmsService.SMS_APPLICATION_PORT_INVALID
          && aSegment.originatorPort !== Ci.nsIGonkSmsService.SMS_APPLICATION_PORT_INVALID) {
        options.originatorPort = aSegment.originatorPort;
      }

      if (options.destinationPort === Ci.nsIGonkSmsService.SMS_APPLICATION_PORT_INVALID
          && aSegment.destinationPort !== Ci.nsIGonkSmsService.SMS_APPLICATION_PORT_INVALID) {
        options.destinationPort = aSegment.destinationPort;
      }
    }

    if (options.receivedSegments < options.segmentMaxSeq) {
      if (DEBUG) {
        debug("Got segment no." + seq + " of a multipart SMS: " +
                           JSON.stringify(options));
      }
      return null;
    }

    
    delete this._receivedSmsSegmentsMap[hash];

    
    if (options.encoding == Ci.nsIGonkSmsService.SMS_MESSAGE_ENCODING_8BITS_ALPHABET) {
      
      
      let fullDataLen = 0;
      for (let i = 1; i <= options.segmentMaxSeq; i++) {
        fullDataLen += options.segments[i].length;
      }

      options.fullData = new Uint8Array(fullDataLen);
      for (let d= 0, i = 1; i <= options.segmentMaxSeq; i++) {
        let data = options.segments[i];
        for (let j = 0; j < data.length; j++) {
          options.fullData[d++] = data[j];
        }
      }
    } else {
      options.fullBody = options.segments.join("");
    }

    
    delete options.receivedSegments;
    delete options.segments;

    if (DEBUG) {
      debug("Got full multipart SMS: " + JSON.stringify(options));
    }

    return options;
  },

  




  _purgeCompleteSmsMessage: function(aMessage) {
    
    delete aMessage.segmentRef;
    delete aMessage.segmentSeq;
    delete aMessage.segmentMaxSeq;

    
    delete aMessage.data;
    delete aMessage.body;
  },

  






  _handleSmsWdpPortPush: function(aMessage, aServiceId) {
    if (aMessage.encoding != Ci.nsIGonkSmsService.SMS_MESSAGE_ENCODING_8BITS_ALPHABET) {
      if (DEBUG) {
        debug("Got port addressed SMS but not encoded in 8-bit alphabet." +
                   " Drop!");
      }
      return;
    }

    let options = {
      bearer: gWAP.WDP_BEARER_GSM_SMS_GSM_MSISDN,
      sourceAddress: aMessage.sender,
      sourcePort: aMessage.originatorPort,
      destinationAddress: this._getPhoneNumber(aServiceId),
      destinationPort: aMessage.destinationPort,
      serviceId: aServiceId
    };
    gWAP.WapPushManager.receiveWdpPDU(aMessage.fullData, aMessage.fullData.length,
                                     0, options);
  },

  _handleCellbroadcastMessageReceived: function(aMessage, aServiceId) {
    gCellBroadcastService
      .notifyMessageReceived(aServiceId,
                             Ci.nsICellBroadcastService.GSM_GEOGRAPHICAL_SCOPE_INVALID,
                             aMessage.messageCode,
                             aMessage.messageId,
                             aMessage.language,
                             aMessage.fullBody,
                             Ci.nsICellBroadcastService.GSM_MESSAGE_CLASS_NORMAL,
                             Date.now(),
                             aMessage.serviceCategory,
                             false,
                             Ci.nsICellBroadcastService.GSM_ETWS_WARNING_INVALID,
                             false,
                             false);
  },

  _handleMwis: function(aMwi, aServiceId) {
    let service = Cc["@mozilla.org/voicemail/voicemailservice;1"]
                  .getService(Ci.nsIGonkVoicemailService);
    service.notifyStatusChanged(aServiceId, aMwi.active, aMwi.msgCount,
                                aMwi.returnNumber, aMwi.returnMessage);

    gRadioInterfaces[aServiceId].sendWorkerMessage("updateMwis", { mwi: aMwi });
  },

  _portAddressedSmsApps: null,
  _handleSmsReceived: function(aMessage, aServiceId) {
    if (DEBUG) debug("_handleSmsReceived: " + JSON.stringify(aMessage));

    if (aMessage.messageType == RIL.PDU_CDMA_MSG_TYPE_BROADCAST) {
      this._handleCellbroadcastMessageReceived(aMessage, aServiceId);
      return true;
    }

    
    
    
    if (aMessage.destinationPort !== Ci.nsIGonkSmsService.SMS_APPLICATION_PORT_INVALID) {
      let handler = this._portAddressedSmsApps[aMessage.destinationPort];
      if (handler) {
        handler(aMessage, aServiceId);
      }
      return true;
    }

    if (aMessage.encoding == Ci.nsIGonkSmsService.SMS_MESSAGE_ENCODING_8BITS_ALPHABET) {
      
      return true;
    }

    aMessage.type = "sms";
    aMessage.sender = aMessage.sender || null;
    aMessage.receiver = this._getPhoneNumber(aServiceId);
    aMessage.body = aMessage.fullBody = aMessage.fullBody || null;

    if (this._isSilentNumber(aMessage.sender)) {
      aMessage.id = -1;
      aMessage.threadId = 0;
      aMessage.delivery = DOM_MOBILE_MESSAGE_DELIVERY_RECEIVED;
      aMessage.deliveryStatus = RIL.GECKO_SMS_DELIVERY_STATUS_SUCCESS;
      aMessage.read = false;

      let domMessage =
        gMobileMessageService.createSmsMessage(aMessage.id,
                                               aMessage.threadId,
                                               aMessage.iccId,
                                               aMessage.delivery,
                                               aMessage.deliveryStatus,
                                               aMessage.sender,
                                               aMessage.receiver,
                                               aMessage.body,
                                               aMessage.messageClass,
                                               aMessage.timestamp,
                                               aMessage.sentTimestamp,
                                               0,
                                               aMessage.read);

      Services.obs.notifyObservers(domMessage,
                                   kSilentSmsReceivedObserverTopic,
                                   null);
      return true;
    }

    if (aMessage.mwiPresent) {
      let mwi = {
        discard: aMessage.mwiDiscard,
        msgCount: aMessage.mwiMsgCount,
        active: aMessage.mwiActive,
        returnNumber: aMessage.sender || null,
        returnMessage: aMessage.fullBody || null
      };

      this._handleMwis(mwi, aServiceId);

      
      
      if (aMessage.mwiDiscard) {
        return true;
      }
    }

    let notifyReceived = (aRv, aDomMessage) => {
      let success = Components.isSuccessCode(aRv);

      this._sendAckSms(aRv, aMessage, aServiceId);

      if (!success) {
        
        
        if (DEBUG) {
          debug("Could not store SMS, error code " + aRv);
        }
        return;
      }

      this._broadcastSmsSystemMessage(
        Ci.nsISmsMessenger.NOTIFICATION_TYPE_RECEIVED, aDomMessage);
      Services.obs.notifyObservers(aDomMessage, kSmsReceivedObserverTopic, null);
    };

    if (aMessage.messageClass != RIL.GECKO_SMS_MESSAGE_CLASSES[RIL.PDU_DCS_MSG_CLASS_0]) {
      gMobileMessageDatabaseService.saveReceivedMessage(aMessage,
                                                        notifyReceived);
    } else {
      aMessage.id = -1;
      aMessage.threadId = 0;
      aMessage.delivery = DOM_MOBILE_MESSAGE_DELIVERY_RECEIVED;
      aMessage.deliveryStatus = RIL.GECKO_SMS_DELIVERY_STATUS_SUCCESS;
      aMessage.read = false;

      let domMessage =
        gMobileMessageService.createSmsMessage(aMessage.id,
                                               aMessage.threadId,
                                               aMessage.iccId,
                                               aMessage.delivery,
                                               aMessage.deliveryStatus,
                                               aMessage.sender,
                                               aMessage.receiver,
                                               aMessage.body,
                                               aMessage.messageClass,
                                               aMessage.timestamp,
                                               aMessage.sentTimestamp,
                                               0,
                                               aMessage.read);

      notifyReceived(Cr.NS_OK, domMessage);
    }

    
    return false;
  },

  


  _sendAckSms: function(aRv, aMessage, aServiceId) {
    if (aMessage.messageClass === RIL.GECKO_SMS_MESSAGE_CLASSES[RIL.PDU_DCS_MSG_CLASS_2]) {
      return;
    }

    let result = RIL.PDU_FCS_OK;
    if (!Components.isSuccessCode(aRv)) {
      if (DEBUG) debug("Failed to handle received sms: " + aRv);
      result = (aRv === Cr.NS_ERROR_FILE_NO_DEVICE_SPACE)
                ? RIL.PDU_FCS_MEMORY_CAPACITY_EXCEEDED
                : RIL.PDU_FCS_UNSPECIFIED;
    }

    gRadioInterfaces[aServiceId]
      .sendWorkerMessage("ackSMS", { result: result });

  },

  








  _smsStorageAvailable: null,
  _reportSmsMemoryStatus: function(aIsAvailable) {
    if (this._smsStorageAvailable !== aIsAvailable) {
      this._smsStorageAvailable = aIsAvailable;
      for (let serviceId = 0; serviceId < gRadioInterfaces.length; serviceId++) {
        gRadioInterfaces[serviceId]
          .sendWorkerMessage("reportSmsMemoryStatus", { isAvailable: aIsAvailable });
      }
    }
  },

  
  _silentNumbers: null,
  _isSilentNumber: function(aNumber) {
    return this._silentNumbers.indexOf(aNumber) >= 0;
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
        this._broadcastSmsSystemMessage(
          Ci.nsISmsMessenger.NOTIFICATION_TYPE_SENT_FAILED, aSendingMessage);
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
                 Ci.nsIIcc.CARD_STATE_READY) {
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
          
          this._broadcastSmsSystemMessage(
            Ci.nsISmsMessenger.NOTIFICATION_TYPE_SENT_FAILED, aDomMessage);
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

  addSilentNumber: function(aNumber) {
    if (this._isSilentNumber(aNumber)) {
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

  


  notifyMessageReceived: function(aServiceId, aSMSC, aSentTimestamp,
                                  aSender, aPid, aEncoding, aMessageClass,
                                  aLanguage, aSegmentRef, aSegmentSeq,
                                  aSegmentMaxSeq, aOriginatorPort,
                                  aDestinationPort, aMwiPresent, aMwiDiscard,
                                  aMwiMsgCount, aMwiActive, aCdmaMessageType,
                                  aCdmaTeleservice, aCdmaServiceCategory,
                                  aBody, aData, aDataLength) {

    this._acquireSmsHandledWakeLock();

    let segment = {};
    segment.iccId = this._getIccId(aServiceId);
    segment.SMSC = aSMSC;
    segment.sentTimestamp = aSentTimestamp;
    segment.timestamp = Date.now();
    segment.sender = aSender;
    segment.pid = aPid;
    segment.encoding = aEncoding;
    segment.messageClass = this._convertSmsMessageClassToString(aMessageClass);
    segment.language = aLanguage;
    segment.segmentRef = aSegmentRef;
    segment.segmentSeq = aSegmentSeq;
    segment.segmentMaxSeq = aSegmentMaxSeq;
    segment.originatorPort = aOriginatorPort;
    segment.destinationPort = aDestinationPort;
    segment.mwiPresent = aMwiPresent;
    segment.mwiDiscard = aMwiDiscard;
    segment.mwiMsgCount = aMwiMsgCount;
    segment.mwiActive = aMwiActive;
    segment.messageType = aCdmaMessageType;
    segment.teleservice = aCdmaTeleservice;
    segment.serviceCategory = aCdmaServiceCategory;
    segment.body = aBody;
    segment.data = (aData && aDataLength > 0) ? aData : null;

    let isMultipart = (segment.segmentMaxSeq && (segment.segmentMaxSeq > 1));
    let messageClass = segment.messageClass;

    let handleReceivedAndAck = (aRvOfIncompleteMsg, aCompleteMessage) => {
      if (aCompleteMessage) {
        this._purgeCompleteSmsMessage(aCompleteMessage);
        if (this._handleSmsReceived(aCompleteMessage, aServiceId)) {
          this._sendAckSms(Cr.NS_OK, aCompleteMessage, aServiceId);
        }
        
      } else {
        this._sendAckSms(aRvOfIncompleteMsg, segment, aServiceId);
      }
    };

    
    if (!isMultipart ||
        (messageClass == RIL.GECKO_SMS_MESSAGE_CLASSES[RIL.PDU_DCS_MSG_CLASS_0])) {
      
      
      
      
      
      
      

      handleReceivedAndAck(Cr.NS_OK,  
                           this._processReceivedSmsSegment(segment));
    } else {
      gMobileMessageDatabaseService
        .saveSmsSegment(segment, (aRv, aCompleteMessage) => {
        handleReceivedAndAck(aRv,  
                             aCompleteMessage);
      });
    }
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
        else if ( aData === kPrefLastKnownSimMcc) {
          gSmsSegmentHelper.enabledGsmTableTuples =
            getEnabledGsmTableTuplesFromMcc();
        }
        break;
      case kDiskSpaceWatcherObserverTopic:
        if (DEBUG) {
          debug("Observe " + kDiskSpaceWatcherObserverTopic + ": " + aData);
        }
        this._reportSmsMemoryStatus(aData != "full");
        break;
      case NS_XPCOM_SHUTDOWN_OBSERVER_ID:
        
        this._releaseSmsHandledWakeLock();
        Services.prefs.removeObserver(kPrefRilDebuggingEnabled, this);
        Services.prefs.removeObserver(kPrefDefaultServiceId, this);
        Services.obs.removeObserver(this, NS_XPCOM_SHUTDOWN_OBSERVER_ID);
        Services.obs.removeObserver(this, kDiskSpaceWatcherObserverTopic);
        break;
    }
  }
};








function getEnabledGsmTableTuplesFromMcc() {
  let mcc;
  try {
    mcc = Services.prefs.getCharPref(kPrefLastKnownSimMcc);
  } catch (e) {}
  let tuples = [[RIL.PDU_NL_IDENTIFIER_DEFAULT,
    RIL.PDU_NL_IDENTIFIER_DEFAULT]];
  let extraTuples = RIL.PDU_MCC_NL_TABLE_TUPLES_MAPPING[mcc];
  if (extraTuples) {
    tuples = tuples.concat(extraTuples);
  }

  return tuples;
};

this.NSGetFactory = XPCOMUtils.generateNSGetFactory([SmsService]);
