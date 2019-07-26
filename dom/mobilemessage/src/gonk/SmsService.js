





"use strict";

const {classes: Cc, interfaces: Ci, utils: Cu, results: Cr} = Components;

Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://gre/modules/Services.jsm");

var RIL = {};
Cu.import("resource://gre/modules/ril_consts.js", RIL);

const RIL_SMSSERVICE_CONTRACTID = "@mozilla.org/sms/rilsmsservice;1";
const RIL_SMSSERVICE_CID =
  Components.ID("{46a9ed78-3574-40a1-9f12-ea179942d67f}");

const DELIVERY_STATE_RECEIVED = "received";
const DELIVERY_STATE_SENDING  = "sending";
const DELIVERY_STATE_SENT     = "sent";
const DELIVERY_STATE_ERROR    = "error";


const kSmsReceivedObserverTopic          = "sms-received";
const kSmsSendingObserverTopic           = "sms-sending";
const kSmsSentObserverTopic              = "sms-sent";
const kSmsFailedObserverTopic            = "sms-failed";
const kSmsDeliverySuccessObserverTopic   = "sms-delivery-success";
const kSmsDeliveryErrorObserverTopic     = "sms-delivery-error";
const kSilentSmsReceivedObserverTopic    = "silent-sms-received";


const kPrefenceChangedObserverTopic = "nsPref:changed";
const kXpcomShutdownObserverTopic   = "xpcom-shutdown";


const kPrefKeyRilDebuggingEnabled = "ril.debugging.enabled";


XPCOMUtils.defineLazyServiceGetter(this, "gIccProvider",
                                   "@mozilla.org/ril/content-helper;1",
                                   "nsIIccProvider");

XPCOMUtils.defineLazyServiceGetter(this, "gMobileMessageDatabaseService",
                                   "@mozilla.org/mobilemessage/rilmobilemessagedatabaseservice;1",
                                   "nsIRilMobileMessageDatabaseService");

XPCOMUtils.defineLazyServiceGetter(this, "gMobileMessageService",
                                   "@mozilla.org/mobilemessage/mobilemessageservice;1",
                                   "nsIMobileMessageService");

XPCOMUtils.defineLazyServiceGetter(this, "gSystemMessenger",
                                   "@mozilla.org/system-message-internal;1",
                                   "nsISystemMessagesInternal");

XPCOMUtils.defineLazyGetter(this, "gRadioInterface", function () {
  let ril = Cc["@mozilla.org/ril;1"].getService(Ci.nsIRadioInterfaceLayer);
  
  return ril.getRadioInterface(0);
});

XPCOMUtils.defineLazyGetter(this, "gPhoneNumberUtils", function () {
  let ns = {};
  Cu.import("resource://gre/modules/PhoneNumberUtils.jsm", ns);
  return ns.PhoneNumberUtils;
});

XPCOMUtils.defineLazyGetter(this, "WAP", function () {
  let WAP = {};
  Cu.import("resource://gre/modules/WapPushManager.js", WAP);
  return WAP;
});


XPCOMUtils.defineLazyGetter(this, "gMessageManager", function () {
  let ns = {};
  Cu.import("resource://gre/modules/RilMessageManager.jsm", ns);
  return ns.RilMessageManager;
});

let DEBUG;
function debug(s) {
  dump("SmsService: " + s + "\n");
}




function SmsService() {
  this._updateDebugFlag();

  this.silentNumbers = [];

  this.portAddressedSmsApps = {};
  this.portAddressedSmsApps[WAP.WDP_PORT_PUSH] =
    this._handleSmsWdpPortPush.bind(this);

  Services.obs.addObserver(this, kPrefenceChangedObserverTopic, false);
  Services.obs.addObserver(this, kXpcomShutdownObserverTopic, false);
}
SmsService.prototype = {

  classID: RIL_SMSSERVICE_CID,
  classInfo: XPCOMUtils.generateCI({classID: RIL_SMSSERVICE_CID,
                                    contractID: RIL_SMSSERVICE_CONTRACTID,
                                    classDescription: "SmsService",
                                    interfaces: [Ci.nsIRilSmsService,
                                                 Ci.nsISmsService],
                                    flags: Ci.nsIClassInfo.SINGLETON}),
  QueryInterface: XPCOMUtils.generateQI([Ci.nsIObserver,
                                         Ci.nsIRilSmsService,
                                         Ci.nsISmsService]),

  




  enabledGsmTableTuples: [
    [RIL.PDU_NL_IDENTIFIER_DEFAULT, RIL.PDU_NL_IDENTIFIER_DEFAULT],
  ],

  




  segmentRef16Bit: false,

  


  segmentRef: 0,
  get nextSegmentRef() {
    let ref = this.segmentRef++;

    this.segmentRef %= (this.segmentRef16Bit ? 65535 : 255);

    
    return ref + 1;
  },

  statusReportPendingMessageIds: null,

  portAddressedSmsApps: null,

  silentNumbers: null,

  _updateDebugFlag: function _updateDebugFlag() {
    try {
      DEBUG = RIL.DEBUG_RIL ||
              Services.prefs.getBoolPref("ril.debugging.enabled");
    } catch (e) {}
  },

  _getStrict7BitEncoding: function _getStrict7BitEncoding() {
    try {
      return Services.prefs.getBoolPref("dom.sms.strict7BitEncoding");
    } catch (e) {
      return false;
    }
  },

  _getRequestStatusReport: function _getRequestStatusReport() {
    try {
      return Services.prefs.getBoolPref("dom.sms.requestStatusReport");
    } catch (e) {
      return true;
    }
  },

  _getMsisdn: function _getMsisdn() {
    let iccInfo = gIccProvider.iccInfo;
    if (!iccInfo) {
      return null;
    }

    let number;
    if (iccInfo.iccType == "ruim") {
      let cdmaIccInfo = iccInfo.QueryInterface(Ci.nsIDOMMozCdmaIccInfo);
      number = cdmaIccInfo.mdn;
    } else {
      let gsmIccInfo = iccInfo.QueryInterface(Ci.nsIDOMMozGsmIccInfo);
      number = gsmIccInfo.msisdn;
    }

    
    
    if (number === undefined || number === "undefined") {
      return null;
    }
    return number;
  },

  

















  _countGsm7BitSeptets: function _countGsm7BitSeptets(text,
                                                      langTable,
                                                      langShiftTable,
                                                      strict7BitEncoding) {
    let length = 0;
    for (let msgIndex = 0; msgIndex < text.length; msgIndex++) {
      let c = text.charAt(msgIndex);
      if (strict7BitEncoding) {
        c = RIL.GSM_SMS_STRICT_7BIT_CHARMAP[c] || c;
      }

      let septet = langTable.indexOf(c);

      
      
      if (septet == RIL.PDU_NL_EXTENDED_ESCAPE) {
        continue;
      }

      if (septet >= 0) {
        length++;
        continue;
      }

      septet = langShiftTable.indexOf(c);
      if (septet < 0) {
        if (!strict7BitEncoding) {
          return -1;
        }

        
        
        c = '*';
        if (langTable.indexOf(c) >= 0) {
          length++;
        } else if (langShiftTable.indexOf(c) >= 0) {
          length += 2;
        } else {
          
          return -1;
        }

        continue;
      }

      
      
      
      if (septet == RIL.PDU_NL_RESERVED_CONTROL) {
        continue;
      }

      
      
      
      
      
      length += 2;
    }

    return length;
  },

  















  _calculateUserDataLength7Bit: function _calculateUserDataLength7Bit(text,
                                                                      strict7BitEncoding) {
    let options = null;
    let minUserDataSeptets = Number.MAX_VALUE;
    for (let i = 0; i < this.enabledGsmTableTuples.length; i++) {
      let [langIndex, langShiftIndex] = this.enabledGsmTableTuples[i];

      const langTable = RIL.PDU_NL_LOCKING_SHIFT_TABLES[langIndex];
      const langShiftTable = RIL.PDU_NL_SINGLE_SHIFT_TABLES[langShiftIndex];

      let bodySeptets = this._countGsm7BitSeptets(text,
                                                  langTable,
                                                  langShiftTable,
                                                  strict7BitEncoding);
      if (bodySeptets < 0) {
        continue;
      }

      let headerLen = 0;
      if (langIndex != RIL.PDU_NL_IDENTIFIER_DEFAULT) {
        headerLen += 3; 
      }
      if (langShiftIndex != RIL.PDU_NL_IDENTIFIER_DEFAULT) {
        headerLen += 3; 
      }

      
      let headerSeptets = Math.ceil((headerLen ? headerLen + 1 : 0) * 8 / 7);
      let segmentSeptets = RIL.PDU_MAX_USER_DATA_7BIT;
      if ((bodySeptets + headerSeptets) > segmentSeptets) {
        headerLen += this.segmentRef16Bit ? 6 : 5;
        headerSeptets = Math.ceil((headerLen + 1) * 8 / 7);
        segmentSeptets -= headerSeptets;
      }

      let segments = Math.ceil(bodySeptets / segmentSeptets);
      let userDataSeptets = bodySeptets + headerSeptets * segments;
      if (userDataSeptets >= minUserDataSeptets) {
        continue;
      }

      minUserDataSeptets = userDataSeptets;

      options = {
        dcs: RIL.PDU_DCS_MSG_CODING_7BITS_ALPHABET,
        encodedFullBodyLength: bodySeptets,
        userDataHeaderLength: headerLen,
        langIndex: langIndex,
        langShiftIndex: langShiftIndex,
        segmentMaxSeq: segments,
        segmentChars: segmentSeptets,
      };
    }

    return options;
  },

  










  _calculateUserDataLengthUCS2: function _calculateUserDataLengthUCS2(text) {
    let bodyChars = text.length;
    let headerLen = 0;
    let headerChars = Math.ceil((headerLen ? headerLen + 1 : 0) / 2);
    let segmentChars = RIL.PDU_MAX_USER_DATA_UCS2;
    if ((bodyChars + headerChars) > segmentChars) {
      headerLen += this.segmentRef16Bit ? 6 : 5;
      headerChars = Math.ceil((headerLen + 1) / 2);
      segmentChars -= headerChars;
    }

    let segments = Math.ceil(bodyChars / segmentChars);

    return {
      dcs: RIL.PDU_DCS_MSG_CODING_16BITS_ALPHABET,
      encodedFullBodyLength: bodyChars * 2,
      userDataHeaderLength: headerLen,
      segmentMaxSeq: segments,
      segmentChars: segmentChars,
    };
  },

  




























  _calculateUserDataLength: function _calculateUserDataLength(text,
                                                              strict7BitEncoding) {
    let options = this._calculateUserDataLength7Bit(text, strict7BitEncoding);
    if (!options) {
      options = this._calculateUserDataLengthUCS2(text);
    }

    if (DEBUG) debug("_calculateUserDataLength: " + JSON.stringify(options));
    return options;
  },

  
















  _fragmentText7Bit: function _fragmentText7Bit(text, langTable, langShiftTable,
                                                segmentSeptets,
                                                strict7BitEncoding) {
    let ret = [];
    let body = "", len = 0;
    for (let i = 0, inc = 0; i < text.length; i++) {
      let c = text.charAt(i);
      if (strict7BitEncoding) {
        c = RIL.GSM_SMS_STRICT_7BIT_CHARMAP[c] || c;
      }

      let septet = langTable.indexOf(c);
      if (septet == RIL.PDU_NL_EXTENDED_ESCAPE) {
        continue;
      }

      if (septet >= 0) {
        inc = 1;
      } else {
        septet = langShiftTable.indexOf(c);
        if (septet == RIL.PDU_NL_RESERVED_CONTROL) {
          continue;
        }

        inc = 2;
        if (septet < 0) {
          if (!strict7BitEncoding) {
            throw new Error("Given text cannot be encoded with GSM 7-bit Alphabet!");
          }

          
          
          c = '*';
          if (langTable.indexOf(c) >= 0) {
            inc = 1;
          }
        }
      }

      if ((len + inc) > segmentSeptets) {
        ret.push({
          body: body,
          encodedBodyLength: len,
        });
        body = c;
        len = inc;
      } else {
        body += c;
        len += inc;
      }
    }

    if (len) {
      ret.push({
        body: body,
        encodedBodyLength: len,
      });
    }

    return ret;
  },

  









  _fragmentTextUCS2: function _fragmentTextUCS2(text, segmentChars) {
    let ret = [];
    for (let offset = 0; offset < text.length; offset += segmentChars) {
      let str = text.substr(offset, segmentChars);
      ret.push({
        body: str,
        encodedBodyLength: str.length * 2,
      });
    }

    return ret;
  },

  

















  _fragmentText: function _fragmentText(text, options, strict7BitEncoding) {
    if (!options) {
      options = this._calculateUserDataLength(text, strict7BitEncoding);
    }

    if (options.dcs == RIL.PDU_DCS_MSG_CODING_7BITS_ALPHABET) {
      const langTable = RIL.PDU_NL_LOCKING_SHIFT_TABLES[options.langIndex];
      const langShiftTable = RIL.PDU_NL_SINGLE_SHIFT_TABLES[options.langShiftIndex];
      options.segments = this._fragmentText7Bit(text,
                                                langTable, langShiftTable,
                                                options.segmentChars,
                                                strict7BitEncoding);
    } else {
      options.segments = this._fragmentTextUCS2(text,
                                                options.segmentChars);
    }

    
    options.segmentMaxSeq = options.segments.length;

    return options;
  },

  








  _broadcastSystemMessage: function _broadcastSystemMessage(aName, aDomMessage) {
    if (DEBUG) debug("Broadcasting the SMS system message: " + aName);

    
    
    
    gSystemMessenger.broadcastMessage(aName, {
      type:           aDomMessage.type,
      id:             aDomMessage.id,
      threadId:       aDomMessage.threadId,
      delivery:       aDomMessage.delivery,
      deliveryStatus: aDomMessage.deliveryStatus,
      sender:         aDomMessage.sender,
      receiver:       aDomMessage.receiver,
      body:           aDomMessage.body,
      messageClass:   aDomMessage.messageClass,
      timestamp:      aDomMessage.timestamp,
      read:           aDomMessage.read
    });
  },

  






  _handleSmsWdpPortPush: function _handleSmsWdpPortPush(message) {
    if (message.encoding != RIL.PDU_DCS_MSG_CODING_8BITS_ALPHABET) {
      if (DEBUG) {
        debug("Got port addressed SMS but not encoded in 8-bit alphabet." +
              " Drop!");
      }
      return;
    }

    let options = {
      bearer: WAP.WDP_BEARER_GSM_SMS_GSM_MSISDN,
      sourceAddress: message.sender,
      sourcePort: message.header.originatorPort,
      destinationAddress: this.rilContext.iccInfo.msisdn,
      destinationPort: message.header.destinationPort,
    };
    WAP.WapPushManager.receiveWdpPDU(message.fullData, message.fullData.length,
                                     0, options);
  },

  _isSilentNumber: function _isSilentNumber(number) {
    return this.silentNumbers.indexOf(number) >= 0;
  },

  



  
  
  hasSupport: function hasSupport() {
    return true;
  },

  getSegmentInfoForText: function getSegmentInfoForText(text, request) {
    let strict7BitEncoding = this._getStrict7BitEncoding();

    let options = this._fragmentText(text, null, strict7BitEncoding);
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

    let result = gMobileMessageService
                 .createSmsSegmentInfo(options.segmentMaxSeq,
                                       options.segmentChars,
                                       options.segmentChars - charsInLastSegment);
    request.notifySegmentInfoForTextGot(result);
  },

  send: function send(number, text, silent, request) {
    let strict7BitEncoding = this._getStrict7BitEncoding();
    let requestStatusReport = this._getRequestStatusReport();

    let options = this._fragmentText(text, null, strict7BitEncoding);
    options.number = gPhoneNumberUtils.normalize(number);
    options.requestStatusReport = requestStatusReport && !silent;
    if (options.segmentMaxSeq > 1) {
      options.segmentRef16Bit = this.segmentRef16Bit;
      options.segmentRef = this.nextSegmentRef;
    }

    let notifyResult = (function notifyResult(rv, domMessage) {
      
      if (!silent) {
        Services.obs.notifyObservers(domMessage, kSmsSendingObserverTopic, null);
      }

      
      
      let errorCode;
      if (!gPhoneNumberUtils.isPlainPhoneNumber(options.number)) {
        if (DEBUG) debug("Error! Address is invalid when sending SMS: " +
                              options.number);
        errorCode = Ci.nsIMobileMessageCallback.INVALID_ADDRESS_ERROR;
      } else if (gRadioInterface.rilContext.radioState !=
                 RIL.GECKO_RADIOSTATE_READY) {
        if (DEBUG) debug("Error! Radio is disabled when sending SMS.");
        errorCode = Ci.nsIMobileMessageCallback.RADIO_DISABLED_ERROR;
      } else if (gRadioInterface.rilContext.cardState !=
                 RIL.GECKO_CARDSTATE_READY) {
        if (DEBUG) debug("Error! SIM card is not ready when sending SMS.");
        errorCode = Ci.nsIMobileMessageCallback.NO_SIM_CARD_ERROR;
      }
      if (errorCode) {
        if (silent) {
          request.notifySendMessageFailed(errorCode);
          return;
        }

        gMobileMessageDatabaseService
          .setMessageDeliveryByMessageId(domMessage.id,
                                         null,
                                         DELIVERY_STATE_ERROR,
                                         RIL.GECKO_SMS_DELIVERY_STATUS_ERROR,
                                         null,
                                         function notifyResult(rv, domMessage) {
          
          request.notifySendMessageFailed(errorCode);
          Services.obs.notifyObservers(domMessage, kSmsFailedObserverTopic, null);
        });
        return;
      }

      
      let context = {
        request: request,
        sms: domMessage,
        requestStatusReport: options.requestStatusReport,
        silent: silent
      };

      
      gRadioInterface.sendWorkerMessage("sendSMS", options,
                                        (function(context, response) {
        if (response.errorMsg) {
          
          let error = Ci.nsIMobileMessageCallback.UNKNOWN_ERROR;
          switch (response.errorMsg) {
            case RIL.ERROR_RADIO_NOT_AVAILABLE:
              error = Ci.nsIMobileMessageCallback.NO_SIGNAL_ERROR;
              break;
          }

          if (context.silent) {
            context.request.notifySendMessageFailed(error);
            return false;
          }

          gMobileMessageDatabaseService
            .setMessageDeliveryByMessageId(context.sms.id,
                                           null,
                                           DELIVERY_STATE_ERROR,
                                           RIL.GECKO_SMS_DELIVERY_STATUS_ERROR,
                                           null,
                                           function notifyResult(rv, domMessage) {
            
            context.request.notifySendMessageFailed(error);
            Services.obs.notifyObservers(domMessage, kSmsFailedObserverTopic, null);
          });
          return false;
        } 

        if (response.deliveryStatus) {
          
          gMobileMessageDatabaseService
            .setMessageDeliveryByMessageId(context.sms.id,
                                           null,
                                           context.sms.delivery,
                                           response.deliveryStatus,
                                           null,
                                           function notifyResult(rv, domMessage) {
            
            let topic = (response.deliveryStatus == RIL.GECKO_SMS_DELIVERY_STATUS_SUCCESS)
                        ? kSmsDeliverySuccessObserverTopic
                        : kSmsDeliveryErrorObserverTopic;
            Services.obs.notifyObservers(domMessage, topic, null);
          });

          
          return false;
        } 

        
        if (context.silent) {
          
          
          
          let sms = context.sms;
          context.request.notifyMessageSent(
            gMobileMessageService.createSmsMessage(sms.id,
                                                   sms.threadId,
                                                   DELIVERY_STATE_SENT,
                                                   sms.deliveryStatus,
                                                   sms.sender,
                                                   sms.receiver,
                                                   sms.body,
                                                   sms.messageClass,
                                                   sms.timestamp,
                                                   sms.read));
          
          return false;
        }

        gMobileMessageDatabaseService
          .setMessageDeliveryByMessageId(context.sms.id,
                                         null,
                                         DELIVERY_STATE_SENT,
                                         context.sms.deliveryStatus,
                                         null,
                                         (function notifyResult(rv, domMessage) {
          
          this._broadcastSystemMessage("sms-sent", domMessage);

          if (context.requestStatusReport) {
            context.sms = domMessage;
          }

          context.request.notifyMessageSent(domMessage);
          Services.obs.notifyObservers(domMessage, kSmsSentObserverTopic, null);
        }).bind(this));

        
        return context.requestStatusReport;
      }).bind(this, context)); 
    }).bind(this); 

    let sendingMessage = {
      type: "sms",
      sender: this._getMsisdn(),
      receiver: number,
      body: text,
      deliveryStatusRequested: options.requestStatusReport,
      timestamp: Date.now()
    };

    if (silent) {
      let deliveryStatus = RIL.GECKO_SMS_DELIVERY_STATUS_PENDING;
      let delivery = DELIVERY_STATE_SENDING;
      let domMessage =
        gMobileMessageService.createSmsMessage(-1, 
                                               0,  
                                               delivery,
                                               deliveryStatus,
                                               sendingMessage.sender,
                                               sendingMessage.receiver,
                                               sendingMessage.body,
                                               "normal", 
                                               sendingMessage.timestamp,
                                               false);
      notifyResult(Cr.NS_OK, domMessage);
      return;
    }

    let id = gMobileMessageDatabaseService.saveSendingMessage(
      sendingMessage, notifyResult);
  },

  addSilentNumber: function addSilentNumber(number) {
    if (this._isSilentNumber(number)) {
      throw Cr.NS_ERROR_UNEXPECTED;
    }

    this.silentNumbers.push(number);
  },

  removeSilentNumber: function removeSilentNumber(number) {
    let index = this.silentNumbers.indexOf(number);
    if (index < 0) {
      throw Cr.NS_ERROR_INVALID_ARG;
    }

    this.silentNumbers.splice(index, 1);
  },

  



  notifyMessageReceived: function notifyMessageReceived(message) {
    if (DEBUG) debug("notifyMessageReceived: " + JSON.stringify(message));

    
    if (message.encoding == RIL.PDU_DCS_MSG_CODING_8BITS_ALPHABET) {
      message.fullData = new Uint8Array(message.fullData);
    }

    
    
    
    if (message.header && message.header.destinationPort != null) {
      let handler = this.portAddressedSmsApps[message.header.destinationPort];
      if (handler) {
        handler(message);
      }
      gRadioInterface.sendWorkerMessage("ackSMS", { result: RIL.PDU_FCS_OK });
      return;
    }

    if (message.encoding == RIL.PDU_DCS_MSG_CODING_8BITS_ALPHABET) {
      
      gRadioInterface.sendWorkerMessage("ackSMS", { result: RIL.PDU_FCS_OK });
      return;
    }

    message.type = "sms";
    message.sender = message.sender || null;
    message.receiver = this._getMsisdn();
    message.body = message.fullBody = message.fullBody || null;
    message.timestamp = Date.now();

    if (this._isSilentNumber(message.sender)) {
      message.id = -1;
      message.threadId = 0;
      message.delivery = DELIVERY_STATE_RECEIVED;
      message.deliveryStatus = RIL.GECKO_SMS_DELIVERY_STATUS_SUCCESS;
      message.read = false;

      let domMessage =
        gMobileMessageService.createSmsMessage(message.id,
                                               message.threadId,
                                               message.delivery,
                                               message.deliveryStatus,
                                               message.sender,
                                               message.receiver,
                                               message.body,
                                               message.messageClass,
                                               message.timestamp,
                                               message.read);

      Services.obs.notifyObservers(domMessage,
                                   kSilentSmsReceivedObserverTopic,
                                   null);
      gRadioInterface.sendWorkerMessage("ackSMS", { result: RIL.PDU_FCS_OK });
      return;
    }

    
    
    
    

    let mwi = message.mwi;
    if (mwi) {
      mwi.returnNumber = message.sender;
      mwi.returnMessage = message.fullBody;
      
      gMessageManager.sendVoicemailMessage("RIL:VoicemailNotification",
                                           this.clientId, mwi);
      gRadioInterface.sendWorkerMessage("ackSMS", { result: RIL.PDU_FCS_OK });
      return;
    }

    let notifyReceived = function notifyReceived(rv, domMessage) {
      let success = Components.isSuccessCode(rv);

      
      gRadioInterface.sendWorkerMessage("ackSMS", {
        result: (success ? RIL.PDU_FCS_OK
                         : RIL.PDU_FCS_MEMORY_CAPACITY_EXCEEDED)
      });

      if (!success) {
        
        
        if (DEBUG) {
          debug("Could not store SMS " + message.id + ", error code " + rv);
        }
        return;
      }

      this._broadcastSystemMessage("sms-received", domMessage);
      Services.obs.notifyObservers(domMessage, kSmsReceivedObserverTopic, null);
    }.bind(this);

    if (message.messageClass == RIL.GECKO_SMS_MESSAGE_CLASSES[RIL.PDU_DCS_MSG_CLASS_0]) {
      message.id = -1;
      message.threadId = 0;
      message.delivery = DELIVERY_STATE_RECEIVED;
      message.deliveryStatus = RIL.GECKO_SMS_DELIVERY_STATUS_SUCCESS;
      message.read = false;

      let domMessage =
        gMobileMessageService.createSmsMessage(message.id,
                                               message.threadId,
                                               message.delivery,
                                               message.deliveryStatus,
                                               message.sender,
                                               message.receiver,
                                               message.body,
                                               message.messageClass,
                                               message.timestamp,
                                               message.read);

      notifyReceived(Cr.NS_OK, domMessage);
      return;
    }

    message.id =
      gMobileMessageDatabaseService.saveReceivedMessage(message,
                                                        notifyReceived);
  },

  



  observe: function observe(subject, topic, data) {
    switch (topic) {
      case kPrefenceChangedObserverTopic:
        if (data === "ril.debugging.enabled") {
          this._updateDebugFlag();
        }
        break;

      case kXpcomShutdownObserverTopic:
        Services.obs.removeObserver(this, kPrefenceChangedObserverTopic);
        Services.obs.removeObserver(this, kXpcomShutdownObserverTopic);
        break;
    }
  }
};

this.NSGetFactory = XPCOMUtils.generateNSGetFactory([SmsService]);
