



"use strict";

const {classes: Cc, interfaces: Ci, utils: Cu, results: Cr} = Components;

Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://gre/modules/Services.jsm");

let MMDB = {};
Cu.import("resource://gre/modules/MobileMessageDB.jsm", MMDB);

const RIL_MOBILEMESSAGEDATABASESERVICE_CONTRACTID =
  "@mozilla.org/mobilemessage/rilmobilemessagedatabaseservice;1";
const RIL_MOBILEMESSAGEDATABASESERVICE_CID =
  Components.ID("{29785f90-6b5b-11e2-9201-3b280170b2ec}");

const DB_NAME = "sms";




function MobileMessageDatabaseService() {
  
  
  Services.dirsvc.get("ProfD", Ci.nsIFile);

  let mmdb = new MMDB.MobileMessageDB();
  mmdb.init(DB_NAME, 0, mmdb.updatePendingTransactionToError.bind(mmdb));
  this.mmdb = mmdb;
}
MobileMessageDatabaseService.prototype = {

  classID: RIL_MOBILEMESSAGEDATABASESERVICE_CID,
  QueryInterface: XPCOMUtils.generateQI([Ci.nsIRilMobileMessageDatabaseService,
                                         Ci.nsIMobileMessageDatabaseService,
                                         Ci.nsIObserver]),

  


  mmdb: null,

  


  observe: function() {},

  



  saveReceivedMessage: function(aMessage, aCallback) {
    this.mmdb.saveReceivedMessage(aMessage, aCallback);
  },

  saveSendingMessage: function(aMessage, aCallback) {
    this.mmdb.saveSendingMessage(aMessage, aCallback);
  },

  setMessageDeliveryByMessageId: function(aMessageId, aReceiver, aDelivery,
                                          aDeliveryStatus, aEnvelopeId,
                                          aCallback) {
    this.mmdb.updateMessageDeliveryById(aMessageId, "messageId", aReceiver,
                                        aDelivery, aDeliveryStatus,
                                        aEnvelopeId, aCallback);
  },

  setMessageDeliveryStatusByEnvelopeId: function(aEnvelopeId, aReceiver,
                                                 aDeliveryStatus, aCallback) {
    this.mmdb.updateMessageDeliveryById(aEnvelopeId, "envelopeId", aReceiver,
                                        null, aDeliveryStatus, null, aCallback);
  },

  setMessageReadStatusByEnvelopeId: function(aEnvelopeId, aReceiver,
                                             aReadStatus, aCallback) {
    this.mmdb.setMessageReadStatusByEnvelopeId(aEnvelopeId, aReceiver,
                                               aReadStatus, aCallback);
  },

  getMessageRecordByTransactionId: function(aTransactionId, aCallback) {
    this.mmdb.getMessageRecordByTransactionId(aTransactionId, aCallback);
  },

  getMessageRecordById: function(aMessageId, aCallback) {
    this.mmdb.getMessageRecordById(aMessageId, aCallback);
  },

  translateCrErrorToMessageCallbackError: function(aCrError) {
    return this.mmdb.translateCrErrorToMessageCallbackError(aCrError);
  },

  saveSmsSegment: function(aSmsSegment, aCallback) {
    this.mmdb.saveSmsSegment(aSmsSegment, aCallback);
  },

  



  getMessage: function(aMessageId, aRequest) {
    this.mmdb.getMessage(aMessageId, aRequest);
  },

  deleteMessage: function(aMessageIds, aLength, aRequest) {
    this.mmdb.deleteMessage(aMessageIds, aLength, aRequest);
  },

  createMessageCursor: function(aHasStartDate, aStartDate, aHasEndDate,
                                aEndDate, aNumbers, aNumbersCount, aDelivery,
                                aHasRead, aRead, aThreadId, aReverse, aCallback) {
    return this.mmdb.createMessageCursor(aHasStartDate, aStartDate, aHasEndDate,
                                         aEndDate, aNumbers, aNumbersCount,
                                         aDelivery, aHasRead, aRead, aThreadId,
                                         aReverse, aCallback);
  },

  markMessageRead: function(aMessageId, aValue, aSendReadReport, aRequest) {
    this.mmdb.markMessageRead(aMessageId, aValue, aSendReadReport, aRequest);
  },

  createThreadCursor: function(aCallback) {
    return this.mmdb.createThreadCursor(aCallback);
  }
};

this.NSGetFactory = XPCOMUtils.generateNSGetFactory([MobileMessageDatabaseService]);
